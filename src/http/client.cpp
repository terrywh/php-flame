#include "../controller.h"
#include "../coroutine.h"
#include "client.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "../time/time.h"

namespace flame::http {
    void client::declare(php::extension_entry& ext) {
        php::class_entry<client> class_client("flame\\http\\client");
        class_client
            .method<&client::__construct>("__construct", {
                {"options", php::TYPE::ARRAY, false, true},
            })
            .method<&client::exec>("exec", {
                {"options", "flame\\http\\client_request"},
            })
            .method<&client::get>("get", {
                {"url", php::TYPE::STRING},
                {"timeout", php::TYPE::INTEGER, false, true}
            })
            .method<&client::post>("post", {
                {"url", php::TYPE::STRING},
                {"body", php::TYPE::UNDEFINED},
                {"timeout", php::TYPE::INTEGER, false, true}
            })
            .method<&client::put>("put", {
                {"url", php::TYPE::STRING},
                {"body", php::TYPE::UNDEFINED},
                {"timeout", php::TYPE::INTEGER, false, true}
            })
            .method<&client::delete_>("delete", {
                {"url", php::TYPE::STRING},
                {"timeout", php::TYPE::INTEGER, false, true}
            });
            ext.add(std::move(class_client));
    }

    void client::check_done() {
        CURLMsg *msg;
        int left;
        client_response* res_;
        while((msg = curl_multi_info_read(c_multi_, &left))) {
            if(msg->msg == CURLMSG_DONE) {
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &res_);
                res_->c_final_ = msg->data.result;
                res_->c_coro_.resume();
            }
        }
    }

    void client::c_socket_ready_cb(const boost::system::error_code& error, curl_socket_t fd, int action, int* expect) {
        if(error == boost::asio::error::operation_aborted || action != *expect) return; // 取消监听或已设置新的监听
        // 不在处理已经关闭的等待
        auto i = c_socks_.find(fd);
        if(i == c_socks_.end()) return;
        // 本次 SOCKET 动作执行
        if(error) action = CURL_CSELECT_ERR;
        curl_multi_socket_action(c_multi_, fd, action, &c_still_);
        check_done();
        if(c_still_ <= 0) c_timer_.cancel();
        if(error || action != *expect) return; // 发生错误或监听被变更
        action = *expect;
        // 未变更时继续监听
        if(action == CURL_POLL_IN || action == CURL_POLL_INOUT) 
            i->second->async_wait(boost::asio::ip::tcp::socket::wait_read, std::bind(&client::c_socket_ready_cb, this, std::placeholders::_1, fd, action, expect));
        if(action == CURL_POLL_OUT || action == CURL_POLL_INOUT) 
            i->second->async_wait(boost::asio::ip::tcp::socket::wait_write, std::bind(&client::c_socket_ready_cb, this, std::placeholders::_1, fd, action, expect));
    }

    int client::c_socket_cb(CURL* e, curl_socket_t fd, int action, void* data, void* sock_data) {
        client* self = reinterpret_cast<client*>(data);
        if(action == CURL_POLL_REMOVE) {
            delete (int*)sock_data;
            curl_multi_assign(self->c_multi_, fd, nullptr);
            return 0;
        }
        int* expect = reinterpret_cast<int*>(sock_data);
        boost::asio::ip::tcp::socket* sock = self->c_socks_[fd];
        if(!sock_data) {
            expect = new int { action };
            curl_multi_assign(self->c_multi_, fd, expect);
        }else{
            *expect = action;
        }
        // 根据新设置重新设置监听
        if(action == CURL_POLL_IN || action == CURL_POLL_INOUT)
            sock->async_wait(boost::asio::ip::tcp::socket::wait_read, std::bind(&client::c_socket_ready_cb, self, std::placeholders::_1, fd, action, expect));
        if(action == CURL_POLL_OUT || action == CURL_POLL_INOUT)
            sock->async_wait(boost::asio::ip::tcp::socket::wait_write, std::bind(&client::c_socket_ready_cb, self, std::placeholders::_1, fd, action, expect));
        return 0;
    }

    int client::c_timer_cb(CURLM *m, long timeout_ms, void* data) {
        client* self = reinterpret_cast<client*>(data);

        self->c_timer_.cancel();
        if(timeout_ms >= 0) {
            self->c_timer_.expires_after(std::chrono::milliseconds(timeout_ms));
            self->c_timer_.async_wait([self] (const boost::system::error_code& err) {
                if(err) return;
                curl_multi_socket_action(self->c_multi_, CURL_SOCKET_TIMEOUT, 0, &self->c_still_);
                self->check_done();
            });
        }
        return 0;
    }

    client::client()
    : c_timer_(gcontroller->context_x) {
        c_multi_ = curl_multi_init();
        curl_multi_setopt(c_multi_, CURLMOPT_SOCKETDATA, this);
        curl_multi_setopt(c_multi_, CURLMOPT_SOCKETFUNCTION, client::c_socket_cb);
        curl_multi_setopt(c_multi_, CURLMOPT_TIMERDATA, this);
        curl_multi_setopt(c_multi_, CURLMOPT_TIMERFUNCTION, client::c_timer_cb);

        curl_multi_setopt(c_multi_, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX | CURLPIPE_HTTP1);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_PIPELINE_LENGTH, 4);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_HOST_CONNECTIONS, 16);
    }

    client::~client() {
        curl_multi_cleanup(c_multi_);
    }

    php::value client::__construct(php::parameters& params) {
        if(params.size() > 0) {
            php::array opts = params[0];
            if(opts.exists("connection_per_host")) {
                int connection_per_host = opts.get("connection_per_host");
                if(connection_per_host < 1 || connection_per_host > 512) 
                    curl_multi_setopt(c_multi_, CURLMOPT_MAX_HOST_CONNECTIONS, connection_per_host);
            }
        }
        return nullptr;
    }

    curl_socket_t client::c_socket_open_cb(void* data, curlsocktype purpose, struct curl_sockaddr* address) {
        boost::system::error_code error;
        boost::asio::ip::tcp::socket* sock = new boost::asio::ip::tcp::socket(gcontroller->context_x);
        if(purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET) {
            sock->open(boost::asio::ip::tcp::v4(), error);
        }else if(purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET6) {
            sock->open(boost::asio::ip::tcp::v6(), error);
        }
        if(error) {
            std::cerr << "[" << flame::time::iso() << "] (ERROR) failed to open socket: " << error.message() << "\n";
            return CURL_SOCKET_BAD;
        }
        curl_socket_t fd = sock->native_handle();
        client* self = reinterpret_cast<client*>(data);
        self->c_socks_.insert({fd, sock});
        return fd;
    }

    int client::c_socket_close_cb(void* data, curl_socket_t fd) {
        client* self = reinterpret_cast<client*>(data);
        auto i = self->c_socks_.find(fd);
        if(i != self->c_socks_.end()) {
            delete i->second;
            self->c_socks_.erase(i);
        }
        return 0;
    }

    php::value client::exec_ex(const php::object& req) {
        auto req_ = static_cast<client_request*>(php::native(req));
        if(req_->c_easy_ == nullptr) req_->c_easy_ = curl_easy_init();
        curl_easy_setopt(req_->c_easy_, CURLOPT_OPENSOCKETFUNCTION, c_socket_open_cb);
        curl_easy_setopt(req_->c_easy_, CURLOPT_OPENSOCKETDATA, this);
        curl_easy_setopt(req_->c_easy_, CURLOPT_CLOSESOCKETFUNCTION, c_socket_close_cb);
        curl_easy_setopt(req_->c_easy_, CURLOPT_CLOSESOCKETDATA, this);
        req_->build_ex();

        php::object res(php::class_entry<client_response>::entry());
        auto res_ = static_cast<client_response*>(php::native(res));
        res_->c_easy_ = req_->c_easy_;
        res_->c_coro_.reset(coroutine::current);
        res_->c_head_ = php::array(4);
        curl_easy_setopt(res_->c_easy_, CURLOPT_WRITEFUNCTION, client_response::c_write_cb);
        curl_easy_setopt(res_->c_easy_, CURLOPT_WRITEDATA, res_);
        curl_easy_setopt(res_->c_easy_, CURLOPT_HEADERFUNCTION, client_response::c_header_cb);
        curl_easy_setopt(res_->c_easy_, CURLOPT_HEADERDATA, res_);
        curl_easy_setopt(res_->c_easy_, CURLOPT_ERRORBUFFER, res_->c_error_);
        curl_easy_setopt(res_->c_easy_, CURLOPT_PRIVATE, res_);

        curl_multi_add_handle(c_multi_, res_->c_easy_);
        res_->c_coro_.suspend(); // <----- check_done

        res_->build_ex();
        curl_multi_remove_handle(c_multi_, res_->c_easy_);
        return std::move(res);
    }
    php::value client::exec(php::parameters& params) {
        return exec_ex(params[0]);
    }
	php::value client::get(php::parameters& params) {
        php::object req(php::class_entry<client_request>::entry());
        req.set("method", "GET");
        req.set("url", params[0]);
        req.set("header", php::array(0));
        req.set("cookie", php::array(0));
        req.set("body", nullptr);
        if(params.length() > 1) {
            req.set("timeout", params[1]);
        }else{
            req.set("timeout", 3000);
        }
        return exec_ex(req);
    }
    php::value client::post(php::parameters& params) {
        php::object req(php::class_entry<client_request>::entry());
        req.set("method",        "POST");
        req.set("url",        params[0]);
        req.set("header", php::array(0));
        req.set("cookie", php::array(0));
        req.set("body",       params[1]);
        if(params.length() > 2) {
            req.set("timeout",params[2]);
        }else{
            req.set("timeout",     3000);
        }
        return exec_ex(req);
    }
    php::value client::put(php::parameters& params) {
        php::object req(php::class_entry<client_request>::entry());
        req.set("method",        "PUT");
        req.set("url",        params[0]);
        req.set("header", php::array(0));
        req.set("cookie", php::array(0));
        req.set("body",       params[1]);
        if(params.length() > 2) {
            req.set("timeout",params[2]);
        }else{
            req.set("timeout",     3000);
        }
        return exec_ex(req);
    }
    php::value client::delete_(php::parameters& params) {
        php::object req(php::class_entry<client_request>::entry());
        req.set("method",         "DELETE");
        req.set("url",        params[0]);
        req.set("header", php::array(0));
        req.set("cookie", php::array(0));
        req.set("body",         nullptr);
        if(params.length() > 1) {
            req.set("timeout", params[1]);
        }else{
            req.set("timeout", 3000);
        }
        return exec_ex(req);
    }
}

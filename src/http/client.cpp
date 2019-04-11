#include "../controller.h"
#include "../coroutine.h"
#include "client.h"
#include "client_poll.h"
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
            if (msg->msg == CURLMSG_DONE) {
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &res_);
                res_->c_final_ = msg->data.result;
                res_->c_coro_.resume();
            }
        }
    }

    static const char* action_str[] = { "UNKNOWN", "CURL_POLL_IN", "CURL_POLL_OUT" , "CURL_POLL_INOUT", "CURL_POLL_REMOVE" };

    void client::c_socket_ready_cb(const boost::system::error_code& error, client_poll* poll, curl_socket_t fd, int action) {
        client* self = reinterpret_cast<client*>(poll->data);
        if (error) action = CURL_CSELECT_ERR;
        curl_multi_socket_action(self->c_multi_, fd, action, &self->c_still_);
        self->check_done();
        if (self->c_still_ <= 0) self->c_timer_.cancel();
    }
    
    int client::c_socket_cb(CURL* e, curl_socket_t fd, int action, void* data, void* sock_data) {
        client* self = reinterpret_cast<client*>(data);
        client_poll* poll = reinterpret_cast<client_poll*>(sock_data);

        if (action == CURL_POLL_REMOVE) {
            curl_multi_assign(self->c_multi_, fd, nullptr);
        }
        else if (poll == nullptr) {
            poll = client_poll::create_poll(gcontroller->context_x, fd, c_socket_ready_cb, self);
            curl_multi_assign(self->c_multi_, fd, poll);
        }
        else {
            // 
        }
        poll->async_wait(action);
        return 0;
    }

    int client::c_timer_cb(CURLM *m, long timeout_ms, void* data) {
        client* self = reinterpret_cast<client*>(data);

        self->c_timer_.cancel();
        if (timeout_ms == 0) timeout_ms = 1;
        if (timeout_ms > 0) {
            self->c_timer_.expires_after(std::chrono::milliseconds(timeout_ms));
            self->c_timer_.async_wait([self] (const boost::system::error_code& err) {
                if (err) return;
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
        // 7.62.x + HTTP1 PIPELINE 功能已无效
        curl_multi_setopt(c_multi_, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX/* | CURLPIPE_HTTP1*/);
        // curl_multi_setopt(c_multi_, CURLMOPT_MAX_PIPELINE_LENGTH, 4L);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_TOTAL_CONNECTIONS, 64L);
        curl_multi_setopt(c_multi_, CURLMOPT_MAXCONNECTS, 32L);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_HOST_CONNECTIONS, 16L);
    }

    client::~client() {
        curl_multi_cleanup(c_multi_);
    }

    php::value client::__construct(php::parameters& params) {
        if (params.size() > 0) {
            php::array opts = params[0];
            if (opts.exists("connection_per_host")) {
                long c = opts.get("connection_per_host");
                if (c < 1 || c > 512) {
                    curl_multi_setopt(c_multi_, CURLMOPT_MAX_TOTAL_CONNECTIONS, c * 4);
                    curl_multi_setopt(c_multi_, CURLMOPT_MAXCONNECTS, c * 2);
                    curl_multi_setopt(c_multi_, CURLMOPT_MAX_HOST_CONNECTIONS, c);
                }
            }
        }
        return nullptr;
    }

    // curl_socket_t client::c_socket_open_cb(void* data, curlsocktype purpose, struct curl_sockaddr* addr) {
    //     client* self = reinterpret_cast<client*>(data);
    //     curl_socket_t fd = ::socket(addr->family, addr->socktype, addr->protocol);
    //     return fd;
    // }

    // int client::c_socket_close_cb(void* data, curl_socket_t fd) {
    //     client* self = reinterpret_cast<client*>(data);
    //     return ::close(fd);
    // }

    php::value client::exec_ex(const php::object& req) {
        auto req_ = static_cast<client_request*>(php::native(req));
        if (req_->c_easy_ == nullptr) req_->c_easy_ = curl_easy_init();
        // curl_easy_setopt(req_->c_easy_, CURLOPT_OPENSOCKETFUNCTION, c_socket_open_cb);
        // curl_easy_setopt(req_->c_easy_, CURLOPT_OPENSOCKETDATA, this);
        // curl_easy_setopt(req_->c_easy_, CURLOPT_CLOSESOCKETFUNCTION, c_socket_close_cb);
        // curl_easy_setopt(req_->c_easy_, CURLOPT_CLOSESOCKETDATA, this);
        curl_easy_setopt(req_->c_easy_, CURLOPT_PIPEWAIT, 1);
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
        if (params.length() > 1) {
            req.set("timeout", params[1]);
        }
        else {
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
        if (params.length() > 2) {
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
        if (params.length() > 2) {
            req.set("timeout",params[2]);
        }
        else {
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
        if (params.length() > 1) {
            req.set("timeout", params[1]);
        }
        else {
            req.set("timeout", 3000);
        }
        return exec_ex(req);
    }
}

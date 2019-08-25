#include "../coroutine.h"
#include "client.h"
#include "../http/client_poll.h"
#include "../time/time.h"
#include "message.h"

namespace flame::smtp {
    
    void client::declare(php::extension_entry& ext) {
        php::class_entry<client> class_client("flame\\smtp\\client");
        class_client
            .method<&client::__construct>("__construct", {}, php::PRIVATE)
            .method<&client::post>("post", {
                {"message", "flame\\smtp\\message"},
            })
            .method<&client::send>("send", {
                {"from", php::TYPE::ARRAY},
                {"to", php::TYPE::ARRAY},
                {"subject", php::TYPE::STRING},
                {"body", php::TYPE::STRING},
            });
        ext.add(std::move(class_client));
    }

    void client::check_done() {
        CURLMsg *msg;
        int left;
        message* req;
        std::vector<message*> rrr;
        while((msg = curl_multi_info_read(c_multi_, &left))) {
            if (msg->msg == CURLMSG_DONE) {
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &req);
                req->c_code_ = msg->data.result;
                req->c_coro_.resume();
            }
        }
    }

    static const char* action_str[] = { "UNKNOWN", "CURL_POLL_IN", "CURL_POLL_OUT" , "CURL_POLL_INOUT", "CURL_POLL_REMOVE" };

    void client::c_socket_ready_cb(const boost::system::error_code& error, http::client_poll* poll, curl_socket_t fd, int action) {
        client* self = reinterpret_cast<client*>(poll->data);
        if (error) action = CURL_CSELECT_ERR;
        curl_multi_socket_action(self->c_multi_, fd, action, &self->c_still_);
        self->check_done();
        if (self->c_still_ <= 0) self->c_timer_.cancel();
    }
    
    int client::c_socket_cb(CURL* e, curl_socket_t fd, int action, void* data, void* sock_data) {
        client* self = reinterpret_cast<client*>(data);
        http::client_poll* poll = reinterpret_cast<http::client_poll*>(sock_data);

        if (action == CURL_POLL_REMOVE) {
            curl_multi_assign(self->c_multi_, fd, nullptr);
        }
        else if (poll == nullptr) {
            poll = http::client_poll::create_poll(gcontroller->context_x, fd, c_socket_ready_cb, self);
            curl_multi_assign(self->c_multi_, fd, poll);
        }
        poll->async_wait(action); // 自行 delete
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

        curl_multi_setopt(c_multi_, CURLMOPT_MAX_TOTAL_CONNECTIONS, 8);
        curl_multi_setopt(c_multi_, CURLMOPT_MAXCONNECTS, 4);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_HOST_CONNECTIONS, 4);

        curl_multi_setopt(c_multi_, CURLMOPT_SOCKETDATA, this);
        curl_multi_setopt(c_multi_, CURLMOPT_SOCKETFUNCTION, client::c_socket_cb);
        curl_multi_setopt(c_multi_, CURLMOPT_TIMERDATA, this);
        curl_multi_setopt(c_multi_, CURLMOPT_TIMERFUNCTION, client::c_timer_cb);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_TOTAL_CONNECTIONS, 64L);
        curl_multi_setopt(c_multi_, CURLMOPT_MAXCONNECTS, 32L);
        curl_multi_setopt(c_multi_, CURLMOPT_MAX_HOST_CONNECTIONS, 16L);
    }

    client::~client() {
        curl_multi_cleanup(c_multi_);
    }

    php::value client::__construct(php::parameters& params) {
        return nullptr;
    }

    php::value client::post_ex(const php::object& req) {
        auto req_ = static_cast<message*>(php::native(req));
        req_->build_ex();
        char error[CURL_ERROR_SIZE];
        curl_easy_setopt(req_->c_easy_, CURLOPT_ERRORBUFFER, error);
        curl_easy_setopt(req_->c_easy_, CURLOPT_PRIVATE, req_);
        curl_easy_setopt(req_->c_easy_, CURLOPT_URL, c_rurl_.c_str());
        curl_easy_setopt(req_->c_easy_, CURLOPT_MAIL_FROM, c_from_.c_str());
        curl_multi_add_handle(c_multi_, req_->c_easy_);
        // curl_multi_socket_action(c_multi_, CURL_SOCKET_TIMEOUT, 0, &c_still_);
        req_->c_coro_.reset(coroutine::current);
        req_->c_coro_.suspend(); // <----- check_done
        curl_multi_remove_handle(c_multi_, req_->c_easy_);

        if(req_->c_code_ != CURLE_OK) {
            throw php::exception(zend_ce_exception,
                (boost::format("Failed to post message: %s / %s") % curl_easy_strerror(req_->c_code_) % error).str(),
                req_->c_code_);
        }
        return true;

        // return 0;
        // long code = 0;
        // curl_easy_getinfo(req_->c_easy_, CURLINFO_RESPONSE_CODE, &code);
        // return code;
    }

    php::value client::post(php::parameters& params) {
        php::object msg = params[0];
        return post_ex(msg);
    }
    
    php::value client::send(php::parameters& params) {
        php::object req(php::class_entry<message>::entry());
        req.call("from", {params[0]});
        req.call("to", {params[1]});
        req.call("subject", {params[2]});
        req.call("append", {"text/html", params[3]});
        return post_ex(req);
    }
}

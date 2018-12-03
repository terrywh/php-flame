#include "../coroutine.h"
#include "client.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "_connection_pool.h"

namespace flame::http {
    void client::declare(php::extension_entry& ext) {
        php::class_entry<client> class_client("flame\\http\\client");
        class_client
            .property({"connection_per_host", 4})
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
                {"body", php::TYPE::INTEGER},
                {"timeout", php::TYPE::INTEGER, false, true}
            })
            .method<&client::put>("put", {
                {"url", php::TYPE::STRING},
                {"body", php::TYPE::INTEGER},
                {"timeout", php::TYPE::INTEGER, false, true}
            })
            .method<&client::delete_>("delete", {
                {"url", php::TYPE::STRING},
                {"timeout", php::TYPE::INTEGER, false, true}
            });
            ext.add(std::move(class_client));
    }
	client::client()
    : cp_(std::make_shared<_connection_pool>(16)) {
    }
    php::value client::__construct(php::parameters& params) {
        if(params.size() > 0) {
            php::array opts = params[0];
            if(opts.exists("connection_per_host")) {
                int cph = opts.get("connection_per_host");
                if(cph > 0 && cph < 512)
                    cp_ = std::make_shared<_connection_pool>(cph);
            }
        }
        cp_->sweep();
        return nullptr;
    }
    php::value client::exec_ex(const php::object& req) {
        coroutine_handler ch{coroutine::current};
        std::cerr << coroutine::current << std::endl;
        auto req_ = static_cast<client_request*>(php::native(req));
        req_->build_ex();

        if(req_->url_->schema.compare("http") == 0) {
            return cp_->execute(req_, static_cast<int>(req_->get("timeout")), ch);
        } /*else if(req_->url_->schema.compare("https") == 0) {*/
            //return cp_->execute(req_, timeout, ch);
        /*}*/
        throw php::exception(zend_ce_exception, (boost::format("HTTP schema \'%1%\' not implemented)") % req_->url_->schema).str());
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

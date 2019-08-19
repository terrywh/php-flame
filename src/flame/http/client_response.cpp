#include "http.h"
#include "value_body.h"
#include "client_response.h"

namespace flame::http {
    void client_response::declare(php::extension_entry& ext) {
        php::class_entry<client_response>  class_client_response("flame\\http\\client_response");
        class_client_response
            .property({"version", "HTTP/1.0"})
            .property({"status", 3000})
            .property({"header", nullptr})
            .property({"body", nullptr})
            .property({"raw_body", ""})
            .method<&client_response::__construct>("__construct", {}, php::PRIVATE)
            .method<&client_response::to_string>("__toString");
        ext.add(std::move(class_client_response));
    }
    // 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
    php::value client_response::__construct(php::parameters& params) 
    {
        return nullptr; 
    }
    client_response::~client_response()
    {
        if (c_easy_) curl_easy_cleanup(c_easy_); // 未执行的请求需要清理
    }
    php::value client_response::to_string(php::parameters& params) {
        return get("raw_body");
    }
    void client_response::build_ex() {
        if (c_final_ != 0) throw php::exception(zend_ce_exception
            , (boost::format("Failed to execute HTTP request: %s") % c_error_).str()
            , c_final_);
        // 响应码
        long rv = 0;
        curl_easy_getinfo(c_easy_, CURLINFO_HTTP_VERSION, &rv);
        set("version", rv == 3 ? "HTTP/2" : rv == 2 ? "HTTP/1.1" : "HTTP/1.0");
        curl_easy_getinfo(c_easy_, CURLINFO_RESPONSE_CODE, &rv);
        set("status", rv);
        // 响应头
        set("header", c_head_);
        // 响应体
        php::string rbody = std::move(c_body_);
        set("raw_body", rbody);
        if (c_head_.exists("content-type")) {
            php::string ctype = c_head_.get("content-type");
            std::string_view sv {ctype.c_str(), ctype.size()};
            set("body", ctype_decode(sv, rbody));
        }
        else {
            set("body", rbody);
        }
    }

    size_t client_response::c_write_cb(char *ptr, size_t size, size_t nmemb, void *data) {
        client_response* self = static_cast<client_response*>(data);
        assert(size == 1);
        self->c_body_.append(ptr, nmemb);
        return nmemb;
    }

    size_t client_response::c_header_cb(char *buffer, size_t size, size_t nitems, void *data) {
        client_response* self = static_cast<client_response*>(data);
        size = size * nitems;       

        std::string_view sv{ buffer, size };
        auto psep = sv.find_first_of(':', 1);
        if (psep != sv.npos) {
            auto pval = sv.find_first_not_of(' ', psep + 1);
            php::string key = php::lowercase(buffer, psep);
            if (pval == sv.npos) {
                self->c_head_.set( key, php::string(0) );
            }
            else {
                self->c_head_.set( key, php::string( buffer + pval, size - pval - 2)); // trailing \r\n
            }
        }
        return size;
    }
}

#include "../../vendor.h"
#include "../../core.h"
#include "../../net/tcp_socket.h"
#include "response.h"
#include "request.h"

namespace net { namespace http {
    std::map<uint32_t, std::string> response::status_list = {
        { 100, "Continue" }, 
        { 101, "Switching Protocols" }, 
        { 200, "OK" }, 
        { 201, "Created" }, 
        { 202, "Accepted" }, 
        { 203, "Non-Authoritative Information" }, 
        { 204, "No Content" }, 
        { 205, "Reset Content" }, 
        { 206, "Partial Content" }, 
        { 300, "Multiple Choices" }, 
        { 301, "Moved Permanently" }, 
        { 302, "Found" }, 
        { 303, "See Other" }, 
        { 304, "Not Modified" }, 
        { 305, "Use Proxy" }, 
        { 307, "Temporary Redirect" }, 
        { 400, "Bad Request" }, 
        { 401, "Unauthorized" }, 
        { 402, "Payment Required" }, 
        { 403, "Forbidden" }, 
        { 404, "Not Found" }, 
        { 405, "Method Not Allowed" }, 
        { 406, "Not Acceptable" }, 
        { 407, "Proxy Authentication Required" }, 
        { 408, "Request Time-out" }, 
        { 409, "Conflict" }, 
        { 410, "Gone" }, 
        { 411, "Length Required" }, 
        { 412, "Precondition Failed" }, 
        { 413, "Request Entity Too Large" }, 
        { 414, "Request-URI Too Large" }, 
        { 415, "Unsupported Media Type" }, 
        { 416, "Requested range not satisfiable" }, 
        { 417, "Expectation Failed" }, 
        { 500, "Internal Server Error" }, 
        { 501, "Not Implemented" }, 
        { 502, "Bad Gateway" }, 
        { 503, "Service Unavailable" }, 
        { 504, "Gateway Time-out" }, 
        { 505, "HTTP Version not supported" }};

    void response::init(php::extension_entry& extension) {
        php::class_entry<response> http_response("flame\\net\\http\\response");
        http_response.add<&response::write_header>("write_header");
        http_response.add<&response::write>("write");
        http_response.add<&response::end>("end");
        http_response.add(php::property_entry("header", nullptr));
        http_response.add<response::build>("build");
        extension.add(std::move(http_response));
        return ;
    }

    php::value response::build(php::parameters& params) {
        php::object req_obj= params[0];
        request* req = req_obj.native<request>();

        php::object rsp_obj = php::object::create<response>();
        response* rsp = rsp_obj.native<response>();
        rsp->req     = req;
        rsp->rsp_hdr.at("content-type", 12) = php::value("text/plain; charset=utf-8", 25);
        rsp->rsp_hdr.at("connection"  , 10) = php::value("close", 5);
        rsp->prop("header") = rsp->rsp_hdr;

        return std::move(rsp_obj);
    }

    void response::set_status_code(int status_code) {
        if(response::status_list.find(status_code) == response::status_list.end()) {
            status_code = 500;      // 默认200 - Internal Server Error
        }
        std::string http_version = std::string(req->prop("version"));
        std::memcpy(header_buffer_.put(http_version.length()), http_version.data(), http_version.length());
        header_buffer_.put(sprintf(header_buffer_.rev(64), " %3d %s\r\n", status_code, response::status_list[status_code].data()));
        return ;
    }

    void response::add_header(const char* key, uint32_t key_len, const char* val, uint32_t val_len) {
        header_buffer_.put(sprintf(header_buffer_.rev(key_len + val_len + 4), "%.*s: %.*s\r\n", key_len, key, val_len, val));
        //fprintf(stderr, "key = [%s], val = [%s]\n", key, val);
        return ;
    }

    php::value response::write_header(php::parameters& params) {
        write_header(params[0]);
        return php::value( [this](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                boost::asio::async_write(req->tcp_->socket_, boost::asio::buffer((char *)header_buffer_, header_buffer_.size()), [this, done](boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        done(nullptr, true);
                    }
                    });
                return nullptr;
                });
    }

    void response::write_header(int status_code) {
        if(header_sended || response_ended) {
            throw php::exception("write header failed: header or response sent");
        }
        header_sended = true;
        set_status_code(status_code);
        php::array head = prop("header");

        for(auto it = head.begin(); it != head.end(); ++it) {
            // if chunked exist ,delete content-lenth
            zend_string* value = it->second.to_string();
            header_buffer_.put(sprintf(header_buffer_.rev(it->first.length() + value->len + 4), 
                        "%.*s: %.*s\r\n", it->first.length(), it->first.data(), value->len, value->val));
        }
        std::memcpy(header_buffer_.put(2), "\r\n", 2);
    }


    php::value response::write(php::parameters& params) {
        if(response_ended) {
            throw php::exception("write failed: reponse sent");
        }
        php::string body = params[0];
        std::vector<boost::asio::const_buffer> buffer_;
        if(!header_sended) {
            write_header(200);
            buffer_.push_back(boost::asio::buffer(header_buffer_.data(), header_buffer_.size()));
        }
        buffer_.push_back(boost::asio::buffer(body.data(), body.length()));
        return php::value( [this, body, buffer_](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                boost::asio::async_write(req->tcp_->socket_, buffer_, [done](boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        done(nullptr, true);
                    }
                    });
                return nullptr;
                });
    }

    php::value response::end(php::parameters& params) {
        if(response_ended) {
            throw php::exception("write failed: reponse sent");
        }
        response_ended = true;
        std::vector<boost::asio::const_buffer> buffer_;
        if(!header_sended) {
            write_header(200);
            buffer_.push_back(boost::asio::buffer(header_buffer_.data(), header_buffer_.size()));
        }
        if(params.length() > 0) {
            php::string body = params[0];
            if(body.length() > 0)
                buffer_.push_back(boost::asio::buffer(body.data(), body.length()));
        }
        if(buffer_.empty())  {
            return nullptr;
        }

        return php::value( [this, buffer_](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                boost::asio::async_write(req->tcp_->socket_, buffer_, [this, done](boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        done(nullptr, nullptr);
                    }
                    });
                return nullptr;
            });
    }
}}

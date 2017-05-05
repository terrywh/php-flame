#include "../../vendor.h"
#include "../../core.h"
#include "../../net/tcp_socket.h"
#include "response.h"

namespace net { namespace http {

    void response::init(php::extension_entry& extension) {
        php::class_entry<response> http_response("flame\\net\\http\\response");
        http_response.add<&response::__construct>("__construct");
        http_response.add<&response::write_header>("write_header");
        http_response.add(php::property_entry("header", nullptr));
        extension.add(std::move(http_response));
        return ;
    }

    php::value response::__construct(php::parameters& params) {
        php::value    tcp_= params[0];
        if(!tcp_.is_object() || !tcp_.is_instance_of<net::tcp_socket>()) {
            throw php::exception("type error: object of flame\\net\\tcp_socket expected");
        }
        socket_ = tcp_.native<net::tcp_socket>();
        php::array header(1);
        header["Content-Type"] = "text/plain;charset=UTF-8";
        this->prop("header") = header;
    }

    void response::set_status_code(int status_code) {
        if(status[status_code].length()) {
            status_code = 500;      // 默认500 - Internal Server Error
        }
        std::memcpy(header_buffer_.put(sizeof("HTTP/1.1 ")), "HTTP/1.1 ", sizeof("HTTP/1.1 "));
        snprintf(header_buffer_.put(4), 4 + 1, "%3d ", status_code);
        std::memcpy(header_buffer_.put(status[status_code].length()), status[status_code].data(), status[status_code].length());
        return ;
    }

    void response::add_header(const char* key, uint32_t key_len, const char* val, uint32_t val_len) {
        std::memcpy(header_buffer_.put(key_len), key, key_len);
        std::memcpy(header_buffer_.put(2), ": ", 2);
        std::memcpy(header_buffer_.put(val_len), val, val_len);
        std::memcpy(header_buffer_.put(2), "\r\n", 2);
        return ;
    }

    php::value response::write_header(php::parameters& params) {
        if(header_sended || response_ended) {
            return nullptr;
        }
        set_status_code(params[0]);
        php::array head = this->prop("header");
        for(auto it = head.begin(); it != head.end(); ++it) {
            zend_string* value = it->second.to_string();
            add_header(it->first.data(), it->first.length(), value->val, value->len);
        }
        std::memcpy(header_buffer_.put(2), "\r\n", 2);

        return php::value( [this](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                boost::asio::async_write(socket_->socket_, boost::asio::buffer((char *)header_buffer_, header_buffer_.size()), [this, done](boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        header_sended = true;
                        done(nullptr, true);
                    }
                    });
                }
        );
    }

    php::value response::write(php::parameters& params) {
        const php::value body = params[0];
        if(!header_sended || response_ended) {
            //throw php::exception("");
            return nullptr;
        }
        return php::value( [this, body](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                boost::asio::async_write(socket_->socket_, boost::asio::buffer((std::string)body), [done](boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        done(nullptr, true);
                    }
                    });
                });
    }

    php::value response::end(php::parameters& params) {
        if(!header_sended) {
            return nullptr;
        }
        const php::value body = params[0];
        return php::value( [this, body](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                boost::asio::async_write(socket_->socket_, boost::asio::buffer((std::string)body), [this, done](boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        response_ended = true;
                        done(nullptr, true);
                    }
                    });
                });
    }
}}

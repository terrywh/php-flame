#include "../../vendor.h"
#include "../../core.h"
#include "../../net/tcp_socket.h"
#include "request.h"
#include "response.h"

namespace net { namespace http {

    void request::init(php::extension_entry& extension) {
        php::class_entry<request> http_request("flame\\net\\http\\request");
        http_request.add<&request::__construct>("__construct");
        http_request.add(php::property_entry("method", nullptr));
        http_request.add(php::property_entry("uri", nullptr));
        http_request.add(php::property_entry("version", nullptr));
        http_request.add(php::property_entry("header", nullptr));
        http_request.add(php::property_entry("query", nullptr));
        http_request.add(php::property_entry("cookie", nullptr));
        http_request.add(php::property_entry("body", nullptr));
        http_request.add<request::parse>("parse");
        extension.add(std::move(http_request));
        return ;
    }

	static void parse_first(char* buffer, std::size_t n, php::object& req) {
		for(int p=0,x=0,i=1;i<n;++i) {
			switch(x) {
			case 0: // METHOD
				if(std::isspace(buffer[i])) {
					req.prop("method") = php::value(php::strtoupper(buffer+p,i-p), i-p);
					p=i+1;
					++x;
				}
			break;
			case 1: // URI
				if(std::isspace(buffer[i])) {
					req.prop("uri") = php::value(buffer+p, i-p);
                    php::value   uri_= req.prop("uri");
                    zend_string* uri = uri_;
                    auto url = php::parse_url(uri->val, uri->len);
                    if(url->query !=nullptr) {
                        req.prop("query") = php::parse_str('&', url->query, std::strlen(url->query));
                    }else{
                        req.prop("query") = php::array((size_t)0);
                    }
                    p=i+1;
					++x;
				}
			break;
			case 2: // VERSION
				if(std::isspace(buffer[i])) {
					req.prop("version") = php::value(php::strtoupper(buffer+p, i-p), i-p);
                    //fprintf(stderr, "version = %.*s\n", i - p, buffer + p);
					// TODO 确认协议形式是否正确？
					return; // 解析结束
				}
			break;
			}
		}
		// TODO 解析失败
	}

	static void parse_header(char* buffer, std::size_t n, php::array& hdr, php::object& req) {
		//php::array item(nullptr);
		//int   len;
        int key_beg, key_end, key_len;
        int val_beg, val_end, val_len;
        char* key;
        char* val;
		for(int x = 0,i = 0; i < n; ++i) {
                
			switch(x) {
			case 0:
				if(!std::isspace(buffer[i]) && buffer[i] != ':') {
					key_beg = i;
					++x;
				}
				break;
			case 1:
				if(!std::isspace(buffer[i]) && buffer[i] != ':') {
                    continue;
				}else if(buffer[i] == ':') {
					key_end = i;
					key_len = key_end - key_beg;
					php::strtolower(buffer + key_beg, key_len);

					key = buffer + key_beg;
                    //item.at(key, len);
					//item.reset( hdr.item(buffer+p, len) );
					//item = php::value("", 0);
					++x;
				}
				break;
			case 2:
				if(!std::isspace(buffer[i])) {
					val_beg = i;
					++x;
				}
				break;
			case 3:
                val_end = i;
				if(!std::isspace(buffer[i])) {
                    continue;
				}else if(buffer[i] == '\r') {
					hdr.at(key, key_len) = php::value(buffer + val_beg, val_end - val_beg);
                    fprintf(stderr, "%.*s = %.*s\n", key_len, key, val_end - val_beg, buffer + val_beg);
					if(std::memcmp(key, "cookie", key_len) == 0) {
						req.prop("cookie") = php::parse_str(';', buffer + val_beg, val_end - val_beg);
					}
                    i+=1;
                    x=0;
				}
			}
		}
        return; // 解析完成
		// TODO 解析失败？
	}

	static std::size_t parse_content_length(php::array& hdr) {
        //php::array h = hdr;
		php::value len = hdr.at("content-length", 14);
		if(len.is_null()) {
			return 0;
		}
		// TODO 限制 BODY 的最大大小？
        return /*std::atoi(static_cast<const char*>(len.to_string()))*/len.to_long();
	}

	void request::parse_request_head(request* r, php::object req, size_t n, php::callable done) {
        buffer_.sgetn(head.put(n), n);

        size_t request_line_len = std::strstr(head.data(), "\r\n") + 2 - head.data();
        parse_first(head.data(), request_line_len, req);

        // parse head
        size_t request_header_len = head.size() - request_line_len;
        parse_header(head.data() + request_line_len, request_header_len, r->hdr, req);
        req.prop("header") = r->hdr;

        // parse body
        if(buffer_.size() > 0) {
            buffer_.sgetn(body.put(buffer_.size()), buffer_.size());
        }

        std::size_t content_length = parse_content_length(r->hdr);
        if(content_length < body.size()) {
            auto body_buff = r->buffer_.prepare(content_length-body.size());
            boost::asio::async_read(r->tcp_->socket_, body_buff, 
                    [r, req, done] (const boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        r->parse_request_body(r, req);
                        done(nullptr, req);
                    }
                    });

        } else if(content_length == 0) {
            done(nullptr, req);
        } else {
            r->parse_request_body(r, req);
            done(nullptr, req);
        }
    }


	void request::parse_request_body(request* r, php::object& req) {
        php::value   type_ = r->hdr.at("content-type", 12);
        zend_string* type = type_;
        php::strtolower(type->val, type->len);
        // 下述两种 content-type 自动解析：
        // 1. application/x-www-form-urlencoded 33
        // 2. application/json 16
        r->buffer_.sgetn(r->body.put(buffer_.size()), buffer_.size());
        if(type->len >=33 && std::memcmp(type->val, "application/x-www-form-urlencoded", 33) == 0) {
            //zend_string* str = body.c_str();
            req.prop("body") = php::parse_str('&', r->body.data(), r->body.size());
        }else if(type->len >=16 && std::memcmp(type->val, "application/json", 16) == 0) {
            //zend_string* str = body.c_str();
            req.prop("body") = php::json_decode(r->body.data(), r->body.size());
        }else{
            // 其他 content-type 返回原始 body 内容
            req.prop("body") = r->body.data();
        }
    }

    php::value request::parse(php::parameters& params) {
        php::value    tcp_= params[0];
        if(!tcp_.is_object() || !tcp_.is_instance_of<net::tcp_socket>()) {
            throw php::exception("type error: object of mill\\net\\tcp_socket expected");
        }

        php::object req = php::object::create<request>(); /*= php::value::object<request>()*/
        request* r = req.native<request>();
        r->tcp_ = tcp_.native<net::tcp_socket>();

        return php::value([r, req](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                    // parse request line
                boost::asio::async_read_until(r->tcp_->socket_, r->buffer_, "\r\n\r\n", 
                    [r, req, done](const boost::system::error_code err, std::size_t n) mutable {
                    if(err == boost::asio::error::eof) {
                        done(nullptr, nullptr);
                    } else if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        r->parse_request_head(r, req, n, done);
                    }
                    });
                return nullptr;
        });

	}
	php::value request::__construct(php::parameters& params) {
		return nullptr;
	}
}}

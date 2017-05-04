#include "../../vendor.h"
#include "../../core.h"
#include "../../net/tcp_socket.h"
#include "request.h"

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
        http_request.add<&request::parse>("parse");
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
        php::array h = hdr;
		php::value len = h.at("content-length", 14);
		if(len.is_null()) {
			return 0;
		}
		// TODO 限制 BODY 的最大大小？
        return /*std::atoi(static_cast<const char*>(len.to_string()))*/len.to_long();
	}

	void request::parse_request_line(php::object& req, size_t n) {
        php::string request_line(n);
        buffer_.sgetn(request_line.data(), n);
        //buffer_.consume(n);
        parse_first(request_line.data(), n, req);
        //char* p = strstr(request_line.data(), "\r\n") + 2;
        //header.rev(request_line.data() - p);
        //size_t len = request_line.data() + request_line.length() - p;
        //if(len) std::memcpy(header.rev(len), p, len);
        size_t len = buffer_.size();
        if(len) {
            buffer_.sgetn(header.put(len), len);
            //buffer_.consume(len);
        }
    }

    void request::parse_request_header(php::object& req, size_t n) {
        // header string 
        buffer_.sgetn(header.put(n), n);
        //buffer_.consume(n);

        // parse header
        parse_header(((zend_string*)header)->val, header.size(), hdr, req);
        req.prop("header") = hdr;
    }

	void request::parse_request_body(php::object& req, size_t n) {
        php::string body(n);
        buffer_.commit(n);
        buffer_.sgetn(body.data(), n);

        php::value   type_ = hdr.at("content-type", 12);
        // 下述两种 content-type 自动解析：
        // 1. application/x-www-form-urlencoded 33
        // 2. application/json 16
        zend_string* type = type_;
        php::strtolower(type->val, type->len);
        if(type->len >=33 && std::memcmp(type->val, "application/x-www-form-urlencoded", 33) == 0) {
            //zend_string* str = body.c_str();
            req.prop("body") = php::parse_str('&', body.data(), body.length());
        }else if(type->len >=16 && std::memcmp(type->val, "application/json", 16) == 0) {
            //zend_string* str = body.c_str();
            req.prop("body") = php::json_decode(body.data(), body.length());
        }else{
            // 其他 content-type 返回原始 body 内容
            req.prop("body") = body;
        }
    }

    php::value request::parse(php::parameters& params) {
        php::value    tcp_= params[0];
        if(!tcp_.is_object() || !tcp_.is_instance_of<net::tcp_socket>()) {
            throw php::exception("type error: object of mill\\net\\tcp_socket expected");
        }

        php::object req = php::object::create<request>(); /*= php::value::object<request>()*/
        request* r = req.native<request>();
        r->tcp = tcp_.native<net::tcp_socket>();

        return php::value([r, req](php::parameters& params) mutable -> php::value {
                php::callable done = params[0];
                    // parse request line
                boost::asio::async_read_until(r->tcp->socket_, r->buffer_, "\r\n", 
                    [r, req, done](const boost::system::error_code err, std::size_t n) mutable {
                    if(err) {
                        done(core::error_to_exception(err));
                    } else {
                        r->parse_request_line(req, n);
                        // parse header
                        boost::asio::async_read_until(r->tcp->socket_, r->buffer_, "\r\n\r\n", 
                            [r, req, done](const boost::system::error_code err, std::size_t n) mutable {
                            if(err) {
                                done(core::error_to_exception(err));
                            } else {
                                r->parse_request_header(req, n);
                                std::size_t content_length = parse_content_length(r->hdr);
                                if(content_length > 0) {
                                    auto body_buff = r->buffer_.prepare(content_length);
                                    // parse body
                                    //boost::asio::async_read_until(tcp, body_buff, "\r\n", 
                                    //[r, req](const boost::system::error_code err, std::size_t n) mutable {
                                    boost::asio::async_read(r->tcp->socket_, body_buff, 
                                            [r, req, done] (const boost::system::error_code err, std::size_t n) mutable {
                                        if(err) {
                                            done(core::error_to_exception(err));
                                        } else {
                                            r->parse_request_body(req, n);
                                            done(nullptr, req);
                                        }
                                        });
                                } else {
                                    done(nullptr, req);
                                }
                            }
                            });

                    }
                    });
        });

	}
	php::value request::__construct(php::parameters& params) {
		return nullptr;
	}
}}

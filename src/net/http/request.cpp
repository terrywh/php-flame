#include "../../vendor.h"
#include "../tcp_socket.h"
#include "request.h"

namespace net { namespace http {

	static void parse_first(char* buffer, std::size_t n, php::value& req) {
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

	static void parse_header(char* buffer, std::size_t n, php::value& hdr, php::value& req) {
		php::value item(nullptr);
		char* key;
		int   len;
		for(int p=0,q=0,x=0,i=0;i<n;++i) {
			switch(x) {
			case 0:
				if(!std::isspace(buffer[i]) && buffer[i] != ':') {
					p=q=i;
					++x;
				}
				break;
			case 1:
				if(!std::isspace(buffer[i]) && buffer[i] != ':') {
					q=i;
				}else if(buffer[i] == ':') {
					len = q-p+1;
					php::strtolower(buffer+p, len);

					key = buffer+p;
					item.reset( hdr.item(buffer+p, len) );
					item = php::value("", 0);
					++x;
				}
				break;
			case 2:
				if(!std::isspace(buffer[i])) {
					p=q=i;
					++x;
				}
				break;
			case 3:
				if(!std::isspace(buffer[i])) {
					q=i;
				}else if(buffer[i] == '\r') {
					item = php::value(buffer+p, q-p+1);
					if(std::memcmp(key, "cookie", len) == 0) {
						req.prop("cookie") = php::parse_str(';', buffer+p, q-p+1);
					}
					++x;
					return; // 解析完成
				}
			}
		}
		// TODO 解析失败？
	}

	static std::size_t parse_content_length(php::value& hdr) {
		php::value len = hdr.offsetGet("content-length", 14);
		if(len.is_null()) {
			return 0;
		}
		// TODO 限制 BODY 的最大大小？
		return std::atoi(static_cast<const char*>(len));
	}

	#ifndef MILL_TCP_BUFLEN
	#define MILL_TCP_BUFLEN (1500 - 68)
	#endif
	static void parse_body(php::buffer& body, std::size_t length, mill_tcpsock sock, std::int64_t dead) {
		std::size_t n;
		while(length > 0) {
			if(length > MILL_TCP_BUFLEN) {
				n = mill_tcprecv(sock, body.put(MILL_TCP_BUFLEN), MILL_TCP_BUFLEN, dead);
			}else if(length > 0) {
				n = mill_tcprecv(sock, body.put(length), length, dead);
			}
			if(errno) {
				throw php::exception("failed to recv request", errno);
			}
			length -= n;
		}
	}

	php::value request::parse(php::parameters& params) {
		php::value tcp_= params[0];
		std::int64_t dead = -1;
		if(params.length()>1) {
			dead = mill_now() + static_cast<std::int64_t>(params[1]);
		}
		if(!tcp_.is_object() || !tcp_.is_instance_of<net::tcp_socket>()) {
			throw php::exception("type error: object of mill\\net\\tcp_socket expected");
		}
		net::tcp_socket* tcp = tcp_.native<net::tcp_socket>();
		php::value req = php::value::object<request>();
		// 解析并填充 request
		// 注意，由于下面 buffer 的限制，HTTP 头每行最大 4096 字节
		char buffer[4096];
		// 1. 第一行 {METHOD} {URI} {HTTP_VERSION}\r\n
		std::size_t n = mill_tcprecvuntil(tcp->socket_, buffer, sizeof(buffer), "\n", 1, dead);
		if(errno == ENOBUFS) {
			throw php::exception("request overflow");
		}
		parse_first(buffer, n, req);
		// 2. query
		php::value   uri_= req.prop("uri");
		zend_string* uri = uri_;
		auto url = php::parse_url(uri->val, uri->len);
		if(url->query !=nullptr) {
			req.prop("query") = php::parse_str('&', url->query, std::strlen(url->query));
		}else{
			req.prop("query") = php::value::array(0);
		}
		// 3. headers
		php::value hdr = php::value::array(0);
		// \r\n 空行时停止
		while((n = mill_tcprecvuntil(tcp->socket_, buffer, sizeof(buffer), "\n", 1, dead)) != 2) {
			parse_header(buffer, n, hdr, req);
		}
		req.prop("header") = hdr;
		// 4. content-length
		// 目前，仅支持 content-length 请求模式
		std::size_t len = parse_content_length(hdr);
		if(len <= 0) {
			// 不存在 body 时，不需要处理下面逻辑
			return std::move(req);
		}
		php::buffer body(MILL_TCP_BUFLEN);
		parse_body(body, len, tcp->socket_, dead);
		// 5. content-type
		php::value   type_= hdr.offsetGet("content-type", 12);
		zend_string* type = type_;
		php::strtolower(type->val, type->len);
		// 下述两种 content-type 自动解析：
		// 1. application/x-www-form-urlencoded 33
		// 2. application/json 16
		if(type->len >=33 && std::memcmp(type->val, "application/x-www-form-urlencoded", 33) == 0) {
			zend_string* str = body.z_str();
			req.prop("body") = php::parse_str('&', str->val, str->len);
		}else if(type->len >=16 && std::memcmp(type->val, "application/json", 16) == 0) {
			zend_string* str = body.z_str();
			req.prop("body") = php::json_decode(str->val, str->len);
		}else{
			// 其他 content-type 返回原始 body 内容
			req.prop("body") = body;
		}
		return std::move(req);
	}
	php::value request::__construct(php::parameters& params) {
		return nullptr;
	}
}}

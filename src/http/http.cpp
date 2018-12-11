#include "../controller.h"
#include "../coroutine.h"
#include "http.h"
#include "client.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "_connection_pool.h"
#include "server.h"
#include "server_request.h"
#include "server_response.h"
#include "_handler.h"

namespace flame::http {
    client* client_;
	std::int64_t body_max_size = 1024 * 1024 * 1024;

	void declare(php::extension_entry& ext) {
		client_ = nullptr;
		ext.on_module_startup([] (php::extension_entry& ext) -> bool {
			// 在框架初始化后创建全局HTTP客户端
            gcontroller->on_init([] (const php::array& opts) {
				body_max_size = std::max(php::ini("post_max_size").calc(), body_max_size);
                client_ = new client();
                php::parameters p(0, nullptr);
                client_->__construct(p);
            })->on_stop([] {
                delete client_;
                client_ = nullptr;
            });
			return true;
		});
		ext
			.function<get>("flame\\http\\get")
			.function<post>("flame\\http\\post")
			.function<put>("flame\\http\\put")
			.function<delete_>("flame\\http\\delete")
			.function<exec>("flame\\http\\exec");

		client_request::declare(ext);
		client_response::declare(ext);
		client::declare(ext);
		server::declare(ext);
		server_request::declare(ext);
		server_response::declare(ext);
	}
	static void init_guard() {
		if(!client_) {
			throw php::exception(zend_ce_parse_error, "flame not initialized");
		}
	}
	php::value get(php::parameters& params) {
		init_guard();
		return client_->get(params);
	}
	php::value post(php::parameters& params) {
		init_guard();
		return client_->post(params);
	}
	php::value put(php::parameters& params) {
		init_guard();
		return client_->put(params);
	}
	php::value delete_(php::parameters& params) {
		init_guard();
		return client_->delete_(params);
	}
	php::value exec(php::parameters& params) {
		init_guard();
		return client_->exec(params);
	}
	php::string ctype_encode(boost::string_view ctype, const php::value& v) {
		if(v.typeof(php::TYPE::STRING)) return v;

		php::value r = v;
		if(ctype.compare(0, 33, "application/x-www-form-urlencoded") == 0) {
			if(r.typeof(php::TYPE::ARRAY)) {
				r = php::callable("http_build_query")({v});
			}else{
				r.to_string();
			}
		}else if(ctype.compare(0, 16, "application/json") == 0) {
			r = php::json_encode(r);
		}else{
			r.to_string();
		}
		return r;
	}
	php::value ctype_decode(boost::string_view ctype, const php::string& v) {
		if(ctype.compare(0, 16, "application/json") == 0) {
			return php::json_decode(v);
		}else if(ctype.compare(0, 33, "application/x-www-form-urlencoded") == 0) {
			php::array data(4);
			php::callable("parse_str")({v, data.make_ref()});
			return data;
		}else if(ctype.compare(0, 19, "multipart/form-data") == 0) {
			std::size_t begin = ctype.find_first_of(';', 19) + 1;
			php::string cache;
			parser::separator_parser<std::string, php::buffer> p1('\0','\0','=','"','"',';', [&cache] (std::pair<std::string, php::buffer> entry) {
				if(entry.first == "boundary" || entry.first == "name") {
					cache = php::string(std::move(entry.second));
				}
			});
			p1.parse(ctype.data() + begin, ctype.data() + ctype.size() - (ctype.data() + begin));
			p1.parse(";", 1); // 结尾分隔符可能不存在(可以认为数据行可能不完整)

			php::array data(4);
			parser::multipart_parser<std::string, php::buffer> p2(cache, [&p1, &cache, &begin, &data] (std::pair<std::string, php::buffer> entry) {
				if(entry.first.size() == 0) {
					data.set(cache, std::move(entry.second));
				}else{
					begin = boost::string_view(entry.second.data(), entry.second.size()).find_first_of(';') + 1;
					p1.parse(entry.second.data() + begin, entry.second.data() + entry.second.size() - (entry.second.data() + begin));
					p1.parse(";", 1);
				}
			});
			p2.parse(v.data(), v.size());

			return data;
		}else{
			return v;
		}
	}
}

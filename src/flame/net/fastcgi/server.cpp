#include "../../fiber.h"
#include "../../process_manager.h"
#include "../net.h"
#include "server.h"
#include "server_connection.h"
#include "server_response.h"

namespace flame {
namespace net {
namespace fastcgi {
	server::server()
	: stream_server(&server_) {

	}
	php::value server::handle(php::parameters& params) {
		if(params.length() >= 2) {
			std::string path  = params[0];
			if(path.length() > 0) {
				handle_map_[path] = params[1];
			}
		}else if (params.length() >= 1) {
			handle_def_ = params[0];
		}else{
			throw php::exception("only Generator function can be use as handler");
		}
		return this;
	}

	php::value server::bind(php::parameters& params) {
		if(params.length() >= 2) {
			unix_socket_ = false;
			php::string& addr = params[0];
			int          port = params[1];
			struct sockaddr_storage address;
			int error = addrfrom(&address, addr.c_str(), port);
			if(error < 0) {
				throw php::exception(uv_strerror(error), error);
			}
			// uv_tcp_init_ex 会创建 socket
			uv_tcp_init_ex(flame::loop, &server_tcp_, address.ss_family);
			// 然后才能进行 SO_REUSEPORT 设置
			enable_socket_reuseport(reinterpret_cast<uv_handle_t*>(&server_tcp_));
			error = uv_tcp_bind(&server_tcp_, reinterpret_cast<struct sockaddr*>(&address), 0);
			if(error < 0) {
				throw php::exception(uv_strerror(error), error);
			}
			// 服务器属性
			prop("local_addresss") = addr + ":" + std::to_string(port);
		}else if(params.length() >= 1) {
			unix_socket_ = true;
			php::string path = params[0];
			if(path.c_str()[0] != '/') {
				throw php::exception("bind failed: only absolute path is allowed");
			}
			uv_pipe_init(flame::loop, &server_pipe_, 0);
			if(flame::process_type == PROCESS_MASTER) {
				// !!! 绑定前需要文件不存在，但此处删除可能会引起其他误会问题
				unlink(path.c_str());
				int error = uv_pipe_bind(&server_pipe_, path.c_str());
				if(error < 0) {
					throw php::exception(uv_strerror(error), error);
				}
			}// 子进程等待主进程传递连接即可
			prop("local_address") = path; // 服务器属性
		}
		return nullptr;
	}
	php::value server::run(php::parameters& params) {
		// 若未指定，提供默认的 handle
		if(handle_def_.is_empty()) {
			handle_def_ = php::value([this] (php::parameters& params) -> php::value {
				php::object& obj = params[1];
				flame::this_fiber()->push([obj] (php::value& rv) {
					// 这里没有什么要做的，这里仅用来保存 obj 的引用
				});
				server_response* res = obj.native<server_response>();
				obj.prop("status")      = 404;
				obj.prop("header_sent") = true;
				obj.prop("ended")       = true;
				res->buffer_head();
				res->buffer_body("router not found", 16);
				res->buffer_ending();
				res->buffer_write();
				return flame::async();
			});
		}
		return unix_socket_ ? stream_server::run_unix(params) : stream_server::run_core(params);
	}
	int server::accept(uv_stream_t* server) {
		server_connection* pobj = new server_connection();
		pobj->svr_ = this;
		if(unix_socket_) {
			uv_pipe_init(flame::loop, &pobj->socket_pipe_, 0);
		}else{
			uv_tcp_init(flame::loop, &pobj->socket_tcp_);
		}
		int error = uv_accept(server, &pobj->socket_);
		if(error < 0) return error;

		pobj->start();
		return 0;
	}
	void server::on_request(php::object& req, php::object& res) {
		php::string& path = req.prop("uri");
		if(path.is_empty()) {
			php::fail("missing 'REQUEST_URI' in webserver config");
			uv_stop(flame::loop);
			exit(-2);
		}else{
			auto hi = handle_map_.find(path);
			if(hi != handle_map_.end()) {
				fiber::start(hi->second, req, res);
			}else{
				fiber::start(handle_def_, req, res);
			}
		}
	}
}
}
}

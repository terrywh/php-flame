#include "../fiber.h"
#include "../process_manager.h"
#include "stream_server.h"

namespace flame {
namespace net {
	stream_server::stream_server(uv_stream_t* s)
	: pstream_(s)
	, running_(false) {

	}
	stream_server::~stream_server() {
		if(running_ && !uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) {
			uv_close(reinterpret_cast<uv_handle_t*>(pstream_), nullptr);
		}
	}
	php::value stream_server::run_core(php::parameters& params) {
		pstream_->data = flame::this_fiber()->push(this);
		int error = uv_listen(reinterpret_cast<uv_stream_t*>(pstream_), 1024, connection_cb_core);
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		running_ = true;
		return flame::async();
	}
	php::value stream_server::run_unix(php::parameters& params) {
		pstream_->data = flame::this_fiber()->push(this);
		running_ = true;
		int error = 0;
		if(flame::process_type == PROCESS_MASTER && flame::manager->is_multiprocess()) {
			error = uv_listen(reinterpret_cast<uv_stream_t*>(pstream_), 1024, connection_cb_unix_master);
		}else if(flame::process_type == PROCESS_WORKER) {
			flame::manager->set_connection_cb(prop("local_address"), connection_cb_unix_worker, this);
		}else{ // 单进程状态与 run_core 一致
			error = uv_listen(reinterpret_cast<uv_stream_t*>(pstream_), 1024, connection_cb_core);
		}
		if(error < 0) {
			throw php::exception(uv_strerror(error), error);
		}
		return flame::async();
	}
	php::value stream_server::close(php::parameters& params) {
		if(!running_ && uv_is_closing(reinterpret_cast<uv_handle_t*>(pstream_))) return nullptr;
		pstream_->data = flame::this_fiber()->push(this);
		uv_close(reinterpret_cast<uv_handle_t*>(pstream_), close_cb);
		return flame::async();
	}
	void stream_server::close_cb(uv_handle_t* handle) {
		flame::fiber*     f = reinterpret_cast<flame::fiber*>(handle->data);
		// stream_server* self = f->context<stream_server>();
		f->next(); // 这里，将当前 server 的运行协程（阻塞在 yield run()）恢复执行
	}
	void stream_server::connection_cb_core(uv_stream_t* server, int error) {
		flame::fiber*     f = reinterpret_cast<flame::fiber*>(server->data);
		stream_server* self = f->context<stream_server>();

		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		error = self->accept(server);
		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
	}
	static void master_send_cb(uv_pipe_t* pipe, void* data) {
		delete pipe; // 已经传送到子进程
	}
	void stream_server::connection_cb_unix_master(uv_stream_t* server, int error) {
		flame::fiber*     f = reinterpret_cast<flame::fiber*>(server->data);
		stream_server* self = f->context<stream_server>();

		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		uv_pipe_t* pipe = new uv_pipe_t;
		uv_pipe_init(flame::loop, pipe, 0);
		error = uv_accept(server, reinterpret_cast<uv_stream_t*>(pipe));
		if(error < 0) {
			delete pipe;
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		flame::manager->send_to_worker(self->prop("local_address"),
			pipe, master_send_cb);
	}
	void stream_server::connection_cb_unix_worker(uv_stream_t* server, int error, void* ctx) {
		stream_server* self = reinterpret_cast<stream_server*>(ctx);
		flame::fiber*     f = reinterpret_cast<flame::fiber*>(self->pstream_->data);
		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
		error = self->accept(server);
		if(error < 0) {
			f->next(php::make_exception(uv_strerror(error), error));
			return;
		}
	}
}
}

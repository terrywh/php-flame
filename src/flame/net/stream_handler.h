#pragma once

namespace flame {
class coroutine;
namespace net {
	template <typename UV_TYPE_T, class MY_SOCKET_T>
	class stream_handler {
	public:
		stream_handler(MY_SOCKET_T* sock)
		: self_(sock) {
			q = buffer_queue_create();
		}
		UV_TYPE_T socket;
		buffer_queue_t q;
		php::value read(php::parameters& params) {
			if(params.length() > 0) {
				if(params[0].is_long()) {
					d_size_ = params[0];
					d_type_ = 1;
					if(d_size_ == 0) d_type_ = 0;
				}else if(params[0].is_string()) {
					d_endl_ = params[0];
					d_type_ = 2;
				}else{
					throw php::exception("read completion condition not supported");
				}
			}else{
				d_type_ = 0;
			}
			if(read()) {
				return rv_;
			}
			socket.data = this;
			obj_ = self_;
			co_  = coroutine::current;
			uv_read_start((uv_stream_t*)&socket, alloc_cb, read_cb);
			return flame::async();
		}
		php::value write(php::parameters& params) {
			write_request_t* ctx = new write_request_t {
				.co  = coroutine::current,
				.obj = self_,
				// .buf = params[0],
			};
			ctx->buf = params[0];
			ctx->req.data = ctx;
			uv_buf_t data {.base = ctx->buf.data(), .len = ctx->buf.length()};
			uv_write(&ctx->req, (uv_stream_t*)&socket, &data, 1, write_cb);
			return flame::async();
		}
		php::value close(php::parameters& params) {
			shutdown_request_t* ctx = new shutdown_request_t {
				.co  = coroutine::current,
				.sh  = this,
				.obj = self_,
			};
			ctx->req.data = ctx;
			uv_shutdown(&ctx->req, (uv_stream_t*)&socket, shutdown_cb);
			return nullptr;
		}
	private:
		typedef struct write_request_t {
			coroutine*  co;
			php::value  obj;
			php::string buf;
			uv_write_t  req;
		} write_request_t;
		typedef struct shutdown_request_t {
			coroutine*      co;
			stream_handler* sh;
			php::value     obj;
			uv_shutdown_t  req;
		} shutdown_request_t;

		MY_SOCKET_T* self_;

		coroutine*  co_;
		php::string rv_;
		php::value  obj_;

		size_t      d_size_;
		php::string d_endl_;
		int         d_type_;

		bool read() {
			switch(d_type_) {
			case 0:
				if(buffer_queue_length(q) > 0) {
					uv_buf_t data = buffer_queue_slice_first(q);
					rv_ = php::string(data.base, data.len);
					buffer_destroy(data);
					return true;
				}
			break;
			case 1:
				if(buffer_queue_length(q) >= d_size_) {
					uv_buf_t data = buffer_queue_slice(q, d_size_);
					rv_ = php::string(data.base, data.len);
					buffer_destroy(data);
					return true;
				}
			break;
			case 2:
			{
				ssize_t ff = buffer_queue_find(q, uv_buf_t {.base = d_endl_.data(), .len = d_endl_.length()});
				if(ff != -1) {
					uv_buf_t data = buffer_queue_slice(q, d_size_);
					rv_ = php::string(data.base, data.len);
					buffer_destroy(data);
					return true;
				}
			}
			break;
			}
			return false;
		}
		static void alloc_cb(uv_handle_t* handle, size_t suggest, uv_buf_t* buf) {
			// static char buffer[2048];
			// buf->base = buffer;
			// buf->len  = sizeof(buffer);
			*buf = buffer_create(2048);
		}
		static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
			auto self = static_cast<stream_handler<UV_TYPE_T, MY_SOCKET_T>*>(handle->data);
			if(nread == UV_EOF || nread == UV_ECANCELED) {
				self->obj_ = nullptr;
				self->co_->next();
			}else if(nread == 0) {

			}else{
				self->obj_ = nullptr;
				buffer_queue_append(self->q, uv_buf_t {.base = buf->base, .len = (size_t)nread});
				if(self->read()) {
					uv_read_stop((uv_stream_t*)&self->socket);
					self->co_->next(self->rv_);
				}
			}
		}
		static void write_cb(uv_write_t* handle, int status) {
			auto ctx = static_cast<write_request_t*>(handle->data);
			ctx->co->next();
			delete ctx;
		}
		static void shutdown_cb(uv_shutdown_t* handle, int status) {
			auto ctx = static_cast<shutdown_request_t*>(handle->data);
			uv_close((uv_handle_t*)&ctx->sh->socket, nullptr);
			ctx->co->next();
			delete ctx;
		}
	};
}
}

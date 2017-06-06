#include "../vendor.h"
#include "../core.h"
#include "server.h"
#include "server_response.h"
#include "header.h"
#include "zlib.h"

namespace http {
	void server_response::init(php::extension_entry& extension) {
		php::class_entry<server_response> class_server_response("flame\\http\\server_response");
		class_server_response.add<&server_response::__destruct>("__destruct");
		class_server_response.add<&server_response::write_header>("write_header", {
			php::of_integer("code"),
			php::of_string("reason"),
		});
		class_server_response.add<&server_response::write>("write", {
			php::of_string("data"),
		});
		class_server_response.add<&server_response::write_file>("write_file", {
			php::of_string("file"),
		});
		class_server_response.add<&server_response::end>("end", {
			php::of_string("data"),
		});
		class_server_response.add<&server_response::enable_gzip>("enable_gzip");
		class_server_response.add(php::property_entry("header", nullptr));
		extension.add(std::move(class_server_response));
	}

	void server_response::init(evhttp_request* evreq, server* svr) {
		req_ = evreq;
		svr_ = svr;
		evhttp_request_set_on_complete_cb(req_, server_response::complete_handler, this);
		php::object hdr_= php::object::create<header>();
        hdr_.native<header>()->init(evhttp_request_get_output_headers(req_));
        hdr = hdr_.native<header>();
		prop("header") = std::move(hdr_);
	}

	server_response::server_response()
	: header_sent_(false)
	, completed_(false)
	, chunk_(evbuffer_new()) {}

	server_response::~server_response() {
		evbuffer_free(chunk_);
        if(is_gzip) deflateEnd(&strm_);
	}

	php::value server_response::__destruct(php::parameters& params) {
		if(!completed_) {
			completed_ = true;
			// 由于当前对象将销毁，不能继续捕捉请求完成的回调
			evhttp_request_set_on_complete_cb(req_, nullptr, nullptr);
			if(!header_sent_) {
				evhttp_send_reply_start(req_, 204, nullptr);
			}
			evhttp_send_reply_end(req_);
		}
		return nullptr;
	}
    php::value server_response::enable_gzip(php::parameters& params) {
        strm_.zalloc = nullptr;
        strm_.zfree  = nullptr;
        strm_.opaque = nullptr;
        strm_.next_in = (Bytef*)zin.data();
        strm_.avail_in = 0;
        // z_stream, level, method, windowBits(gzip => 15 < n < 32), memLevel(default = 1), strategy
        int ret = deflateInit2(&strm_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 1, Z_DEFAULT_STRATEGY);
        if(ret != Z_OK) {
            throw php::exception("deflate init failure", ret);
        }
        evhttp_add_header(hdr->queue_, "Content-Encoding", "gzip");
        //evhttp_add_header(hdr->queue_, "Content-Type", "text/html");
        is_gzip = true;
    }

	php::value server_response::write_header(php::parameters& params) {
		if(header_sent_) throw php::exception("write_header failed: header already sent");
		if(params.length() > 1) {
			zend_string* reason = params[1];
            evhttp_send_reply_start(req_, params[0], reason->val);
		}else{
            evhttp_send_reply_start(req_, params[0], nullptr);
		}
        header_sent_ = true;
		// libevent 没有提供 evhttp_send_reply_start 完成回调的功能，
		// 参见：v2.1.8 http.c:2838
		return nullptr;
	}

#define OUT_LEN 512
    // TODO 内存使用优化
    int32_t server_response::gzip_add() {
        if(zin.length() == 0) return 0;
        zout.reset();
        uint32_t len;
        strm_.next_in = (Bytef *)zin.data();
        strm_.avail_in = zin.length();

        do {
            char out[OUT_LEN];
            strm_.next_out  = reinterpret_cast<Bytef*>(out);
            strm_.avail_out = OUT_LEN;
            int32_t ret = deflate(&strm_, Z_NO_FLUSH);
            if(ret == Z_BUF_ERROR) {
                std::printf("buffer error is not fatal, deflate can call again to continue compressing\n");
            } else if (ret < 0){
                return ret;
            }
            uint32_t len = OUT_LEN - strm_.avail_out;
            if(len > 0) memcpy(zout.put(len), out, len);
        } while(strm_.avail_in != 0);

        return len;
    }

    // TODO 内存使用优化
    int32_t server_response::gzip_end() {
        if(zin.length() != 0) {
            strm_.next_in = (Bytef*)zin.data();
            strm_.avail_in = zin.length();
        } else {
            strm_.next_in = nullptr;
            strm_.avail_in = 0;
        }
        zout.reset();
        int32_t ret = 0;
        do {
            char out[OUT_LEN];
            strm_.next_out  = reinterpret_cast<Bytef*>(out);
            strm_.avail_out = OUT_LEN;

            ret = deflate(&strm_, Z_FINISH);
            if(ret == Z_BUF_ERROR) {
                std::printf("buffer error is not fatal, deflate can call again to continue compressing\n");
            } else if (ret < 0) {
                return ret;
            }
            uint32_t len = OUT_LEN - strm_.avail_out;
            if(len > 0) {
                memcpy(zout.put(len), out, len);
            }
        } while(ret != Z_STREAM_END);
        return ret;
    }

	php::value server_response::write(php::parameters& params) {
		if(completed_)       throw php::exception("write failed: response aready ended");

        if(is_gzip) {
            zin = params[0];
            int32_t ret = gzip_add();
            if(ret < 0)          throw php::exception("gzip error: unknown exception", ret);
            if(zout.size() == 0)
                return nullptr;
            wbuffer_.push_back(php::string(zout.data(), zout.size()));
        } else {
            wbuffer_.push_back(params[0]);
        }

		php::string& data = wbuffer_.back();
		evbuffer_add_reference(chunk_, data.data(), data.length(), nullptr, nullptr);
		return php::value([this] (php::parameters& params) -> php::value {
			cb_ = params[0];
			if(!header_sent_) {
				evhttp_send_reply_start(req_, 200, nullptr);
                header_sent_ = true;
			}
			evhttp_send_reply_chunk_with_cb(req_, chunk_,
				reinterpret_cast<void (*)(struct evhttp_connection*, void*)>(server_response::complete_handler), this);
			return nullptr;
		});
	}

	void server_response::complete_handler(struct evhttp_request* _, void* ctx) {
		// 注意：上面 evhttp_request* 指针不能使用（有强制类型转换的释放方式）
		server_response* self = reinterpret_cast<server_response*>(ctx);
		// 发送完毕后需要清理缓存（参数引用）
		self->wbuffer_.clear();
		// 通知 server 请求完成（server 关闭流程）
		self->svr_->request_finish();
		if(self->cb_.is_empty()) return;
		// callback 调用机制请参考 tcp_socket 内相关说明
		php::callable cb = std::move(self->cb_);
		cb();
	}

	php::value server_response::write_file(php::parameters& params) {
		if(completed_) throw php::exception("write_file failed: response aready ended");
		if(header_sent_) throw php::exception("write_file failed: header aready sent");
		zend_string* file = params[0];
		int fd = ::open(file->val, O_RDONLY);
		if(fd == -1) {
			throw php::exception(
				(boost::format("write_file failed: %s") % strerror(errno)).str(),
				errno);
		}
		struct stat st;
		if(fstat(fd, &st) == -1) {
			throw php::exception(
				(boost::format("write_file failed: %s") % strerror(errno)).str(),
				errno);
		}
		if(st.st_size <= 0 || evbuffer_add_file(chunk_, fd, 0, st.st_size) == -1) {
			throw php::exception("write_file failed: EBADF", EBADF);
		}
		return php::value([this] (php::parameters& params) -> php::value {
			completed_ = true;
			cb_ = params[0];
			evhttp_send_reply(req_, 200, "OK", chunk_);
			return nullptr;
		});
	}

	php::value server_response::end(php::parameters& params) {
		if(completed_)  throw php::exception("end failed: response already ended");
        if(is_gzip) {
            zin = params[0];
            int32_t ret = gzip_end();
            if(ret < 0) throw php::exception("gzip error: unknown exception", ret);
            wbuffer_.push_back(php::string(zout.data(), zout.size()));
            php::string& data = wbuffer_.back();
            evbuffer_add_reference(chunk_, data.data(), data.length(), nullptr, nullptr);
        } else if(params.length() > 0) {
            wbuffer_.push_back(params[0]);
            php::string& data = wbuffer_.back();
            evbuffer_add_reference(chunk_, data.data(), data.length(), nullptr, nullptr);
        }
		return php::value([this] (php::parameters& params) -> php::value {
			completed_ = true;
			cb_ = params[0];
			if(!header_sent_) {
				evhttp_send_reply_start(req_, 200, nullptr);
                header_sent_ = true;
			}
			evhttp_send_reply_chunk(req_, chunk_);
			evhttp_send_reply_end(req_);
			return nullptr;
		});
	}
}

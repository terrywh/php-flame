#pragma once
#include "process.h"
#include "../util/pipe_parser.h"

namespace flame {
	class process_worker: public process {
	public:
		virtual void init() override;
		virtual void run() override;
	private:
		uv_loop_t   loop_;
		// 通讯管道
		uv_pipe_t   pipe_;
		util::pipe_parser pipe_parser_;
		static void on_pipe_read(uv_stream_t* stream, ssize_t n, const uv_buf_t* buf);
	};
}

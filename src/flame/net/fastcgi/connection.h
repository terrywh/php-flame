#pragma once

namespace flame {
namespace net {
namespace fastcgi {
	class connection {
	private:
		uv_pipe_t socket_;
		
		enum {
			// VERSION
			PV_VERSION = 1,
			// TYPE
			PT_BEGIN_REQUEST = 1,
			PT_ABORT_REQUEST = 2,
			PT_END_REQUEST   = 3,
			PT_PARAMS        = 4,
			PT_STDIN         = 5,
			PT_STDOUT        = 6,
			PT_STDERR        = 7,
			PT_DATA          = 8,
			PT_GET_VALUES    = 9,
			PT_GET_VALUES_RESULT = 10,
			PT_UNKNOWN_TYPE  = 11,
			// FLAGS
			PF_KEEP_CONN = 0x01,
			// ROLE
			PR_RESPONDER  = 1,
			PR_AUTHORIZER = 2,
			PR_FILTER     = 3,
			// STATUS
			// ---------------------------------
			// 1. RECORD
			PS_RECORD_VERSION,
			PS_RECORD_TYPE,
			PS_RECORD_REQUEST_ID_1,
			PS_RECORD_REQUEST_ID_2,
			PS_RECORD_CONTENT_LEN_1,
			PS_RECORD_CONTENT_LEN_2,
			PS_RECORD_PADDING_LEN,
			PS_RECORD_RESERVED,
			// 2. BEGIN_REQUEST
			PS_BODY_1_ROLE_1,
			PS_BODY_1_ROLE_2,
			PS_BODY_1_FLAGS,
			PS_BODY_1_RESERVED_1,
			PS_BODY_1_RESERVED_2,
			PS_BODY_1_RESERVED_3,
			PS_BODY_1_RESERVED_4,
			PS_BODY_1_RESERVED_5,
			// 3. PARAMS
			PS_BODY_2_KEY_LEN_1,
			PS_BODY_2_KEY_LEN_2,
			PS_BODY_2_KEY_LEN_3,
			PS_BODY_2_KEY_LEN_4,
			PS_BODY_2_KEY_DATA,
			PS_BODY_2_VAL_LEN_1,
			PS_BODY_2_VAL_LEN_2,
			PS_BODY_2_VAL_LEN_3,
			PS_BODY_2_VAL_LEN_4,
			PS_BODY_2_VAL_DATA,
			// 4. STDIN
			PS_BODY_3_DATA,
			// 5. RECORD
			PS_RECORD_PADDING,
			// ---------------------------------
		};
		int status_;

		unsigned short clen_;
		unsigned char  plen_;
		unsigned int   klen_;
		unsigned int   vlen_;
		php::buffer    key_;
		php::buffer	   val_; // 缓冲还未接收完成的数据

		void start();
		char buffer_[8192];
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
		static void close_cb(uv_handle_t* handle);
		int parse(const char* data, int size);

	public:
		unsigned char  type;
		unsigned short role;
		int            flag;

		friend class server;
	};
}
}
}

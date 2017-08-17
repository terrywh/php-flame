#pragma once

namespace flame {
namespace net {
namespace http {
	class server_request;
}
namespace fastcgi {
	class server;
	class server_connection {
	private:
		uv_pipe_t socket_;
		server*   server_;

		enum {
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

		multipart_parser_settings mps_;
		multipart_parser*         mpp_;

		void start();
		void close();
		char buffer_[8192];
		static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
		static void close_cb(uv_handle_t* handle);
		static int mp_beg_cb(multipart_parser*);
		static int mp_key_cb(multipart_parser*, const char *at, size_t length);
		static int mp_val_cb(multipart_parser*, const char *at, size_t length);
		static int mp_hdr_cb(multipart_parser*);
		static int mp_dat_cb(multipart_parser*, const char *at, size_t length);
		static int mp_end_cb(multipart_parser*);

		int parse(const char* data, int size);

		php::object           obj_;
		php::array*           hdr_;
		php::array*           bdy_;
	public:
		unsigned char  type;
		unsigned short role;
		int            flag;
		unsigned short request_id;

		friend class server;
		friend class server_response;
	};
}
}
}

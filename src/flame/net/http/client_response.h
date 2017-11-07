#pragma once

namespace flame {
namespace net {
namespace http {
	class client_response: public php::class_base {
	public:
		client_response();
		// header
		// body
		php::value to_string(php::parameters& params) {
			return prop("body", 4);
		}
		// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
		php::value __construct(php::parameters& params) {
			return nullptr;
		}

	private:
		void head_cb(char* header, size_t size);
		void body_cb(char* body, size_t size);
		void done_cb(CURLMsg* msg);
		php::buffer        body_;
		const char*        key_data; // 缓存当前 key 指针
		size_t             key_size;
		php::array         header_;
		php::array         cookie_;
		php::array         cookie_item;
		kv_parser          header_parser;
		kv_parser_settings header_parser_conf;
		static int header_key_cb(kv_parser*, const char*, size_t);
		static int header_val_cb(kv_parser*, const char*, size_t);
		kv_parser          cookie_parser;
		kv_parser_settings cookie_parser_conf;
		static int cookie_key_cb(kv_parser*, const char*, size_t);
		static int cookie_val_cb(kv_parser*, const char*, size_t);
		
		friend class client;
	};
}
}
}

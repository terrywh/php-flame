#pragma once

namespace net {
    class tcp_socket;
    namespace http {
	class response: public php::class_base {
	public:
        static void init(php::extension_entry& extension);
		php::value __construct(php::parameters& params);
        php::value write_header(php::parameters& params);
        php::value write(php::parameters& params);
        php::value end(php::parameters& params);
    private:
        void set_status_code(int status_code);
        void add_header(const char* key, uint32_t key_len, const char* val, uint32_t val_len);

    private:
        bool header_sended;
        bool response_ended;
        php::buffer header_buffer_;

    private:
        static std::map<uint32_t, std::string> status;
        net::tcp_socket* socket_;
	};
} }

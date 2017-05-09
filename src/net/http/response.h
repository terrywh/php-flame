#pragma once

namespace net {
    class tcp_socket;
    namespace http {
	class response: public php::class_base {
	public:
        static void init(php::extension_entry& extension);
        static php::value build(php::parameters& params);
		php::value __construct(php::parameters& params);
        php::value write_header(php::parameters& params);
        void       write_header(int status_code);
        php::value write(php::parameters& params);
        php::value end(php::parameters& params);
    private:
        void set_status_code(int status_code);
        void add_header(const char* key, uint32_t key_len, const char* val, uint32_t val_len);

    private:
        request* req;
        bool header_sended;
        bool response_ended;
        php::buffer header_buffer_;
        php::array  rsp_hdr;

    private:
        static std::map<uint32_t, std::string> status_list;

        friend class request;
	};

} }

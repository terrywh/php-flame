#pragma once

namespace net { 
    class tcp_socket;
    namespace http {
	class request: public php::class_base {
	public:
        //request() = default ;
        static void init(php::extension_entry& extension);
		static php::value parse(php::parameters& params);
		php::value __construct(php::parameters& params);
        void parse_request_line(php::object& req, size_t n);
        void parse_request_header(php::object& req, size_t n);
        void parse_request_body(php::object& req, size_t n);

    private:
        php::buffer header;
        php::array  hdr;
        boost::asio::streambuf buffer_;
        net::tcp_socket* tcp;
        php::value done;
	};
} }

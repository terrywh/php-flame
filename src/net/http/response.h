#pragma once

namespace net {
	class tcp_socket;
namespace http {
	class write_buffer;
	class request;
	class response: public php::class_base {
	public:
		response()
		: header_(std::size_t(0))
		, header_sent_(false)
		, ended_(false) {
			buffer_.reserve(64);
		}
		static void init(php::extension_entry& extension);
		static php::value build(php::parameters& params);
		php::value write_header(php::parameters& params);
		php::value write(php::parameters& params);
		php::value end(php::parameters& params);
	private:
		void build_header(int status_code);

		tcp::socket* socket_;
		bool         header_sent_;
		bool         ended_;
		// php::buffer  header_buffer_;
		php::array   header_;
		std::vector<write_buffer> buffer_;
		static std::map<uint32_t, std::string> status_map;

		friend class request;
	};
} }

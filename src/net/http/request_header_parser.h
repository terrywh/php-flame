#pragma once

namespace net { namespace http {
	class request;
	class request_header_parser {
	private:
		enum {
			METHOD_BEFORE,
			METHOD,
			METHOD_AFTER_1,
			PATH_BEFORE,
			PATH,
			QUERY,
			QUERY_AFTER_1,
			VERSION_BEFORE,
			VERSION,
			VERSION_AFTER_1, // \r
			VERSION_AFTER_2, // \n
			HEADER_FIELD,
			HEADER_SEPERATOR, // :
			HEADER_VALUE_BEFORE, // OWS
			HEADER_VALUE,
			HEADER_VALUE_AFTER_1, // \r
			HEADER_VALUE_AFTER_2, // \n
			HEADER_COMPLETE_1, // \r
			HEADER_COMPLETE_2, // \n
		};
	public:
		request_header_parser(request* req, php::array& hdr)
			: status_(METHOD_BEFORE)
			, req_(req)
			, hdr_(hdr) {}

		bool parse(boost::asio::streambuf& buffer, std::size_t n);
		tribool parse(char c);
		bool complete() {
			return status_ == HEADER_COMPLETE_2;
		}
	private:
		int         status_;
		request*    req_;
		php::array& hdr_;
		// 缓存解析数据
		std::string field_;
		std::string value_;
	};
}}

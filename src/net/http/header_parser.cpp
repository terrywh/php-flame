#include "../../vendor.h"
#include "header_parser.h"
#include "server_request.h"

namespace net { namespace http {

	bool header_parser::parse(boost::asio::streambuf& buffer, std::size_t n) {
		while(n > 0) {
			char c = buffer.sbumpc();
REPEAT:
			auto r = parse(c);
			if(r) {
				--n;
			}else if(!r) {
				return false;
			}else{
				goto REPEAT;
			}
		}
		return true;
	}
	tribool header_parser::parse(char c) {
		switch(status_) {
		case METHOD_BEFORE:
			if(c == ' ') return false;
			status_ = METHOD;
			value_.push_back(c);
			break;
		case METHOD:
			if(std::isspace(c)) {
				status_ = METHOD_AFTER_1;
				return indeterminate;
			}
			value_.push_back(c);
			break;
		case METHOD_AFTER_1:
		 	boost::algorithm::to_upper(value_);
			req_->prop("method") = value_;
			value_.clear();
			status_ = PATH_BEFORE;
			break;
		case PATH_BEFORE:
			if(c == ' ') return false;
			status_ = PATH;
			field_.push_back(c);
			break;
		case PATH:
			if(c == '?') {
				status_ = QUERY;
			}else if(c == ' ') {
				status_ = QUERY_AFTER_1;
				return indeterminate;
			}else{
				field_.push_back(c);
			}
			break;
		case QUERY:
			if(c == ' ') {
				status_ = QUERY_AFTER_1;
				return indeterminate;
			}
			value_.push_back(c);
			break;
		case QUERY_AFTER_1:
			req_->prop("path") = field_;
			if(value_.length()) {
				req_->prop("get")  = php::parse_str('&', value_.c_str(), value_.length());
			}
			field_.clear();
			value_.clear();
			status_ = VERSION_BEFORE;
			break;
		case VERSION_BEFORE:
			if(c != 'H') return false;
			value_.push_back(c);
			status_ = VERSION;
			break;
		case VERSION:
			if(c == '\r') {
				status_ = VERSION_AFTER_1;
				return indeterminate;
			}
			value_.push_back(c);
			break;
		case VERSION_AFTER_1:
			req_->prop("version") = value_;
			value_.clear();
			status_ = VERSION_AFTER_2;
			break;
		case VERSION_AFTER_2:
			if(c != '\n') return false;
			status_ = HEADER_FIELD;
			break;
		case HEADER_FIELD:
			if(c == '\r') {
				status_ = HEADER_COMPLETE_1;
				return indeterminate;
			}else if(c == ':') {
				status_ = HEADER_SEPERATOR;
				return indeterminate;
			}else if(std::isspace(c)) {
				return false;
			}
			field_.push_back(c);
			break;
		case HEADER_SEPERATOR:
			status_ = HEADER_VALUE_BEFORE;
			break;
		case HEADER_VALUE_BEFORE:
			if(!std::isspace(c)) {
				value_.push_back(c);
				status_ = HEADER_VALUE;
			}
			break;
		case HEADER_VALUE: // !!! RFC 7230 允许 VALUE 存在 尾部空白，这里没有处理
			if(c == '\r') {
				status_ = HEADER_VALUE_AFTER_1;
				return indeterminate;
			}
			value_.push_back(c);
			break;
		case HEADER_VALUE_AFTER_1:
			status_ = HEADER_VALUE_AFTER_2;
			boost::algorithm::to_lower(field_);
			hdr_[field_] = value_;
			if(field_ == "cookie") {
				req_->prop("cookie") = php::parse_str(';', value_.c_str(), value_.length());
			}
			field_.clear();
			value_.clear();
			break;
		case HEADER_VALUE_AFTER_2:
			if(c != '\n') return false;
			status_ = HEADER_FIELD;
			break;
		case HEADER_COMPLETE_1:
			status_ = HEADER_COMPLETE_2;
			break;
		case HEADER_COMPLETE_2:
			if(c != '\n') return false;
			break;
		}
		return true;
	}
}}

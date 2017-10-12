#include "pipe_parser.h"

namespace util {
	size_t pipe_parser::execute(const char* data, size_t size) {
		char   c;
		size_t i = 0;
		while(i < size) {
			c = data[i];
			switch(status_) {
			case PPS_SIZE_1:
				*(char*)&size_ = c;
				status_ = PPS_SIZE_2;
			break;
			case PPS_SIZE_2:
				*(((char*)&size_) + 1) = c;
				status_ = PPS_TYPE;
			break;
			case PPS_TYPE:
				type_ = c;
				status_ = PPS_RESERVED;
			break;
			case PPS_RESERVED:
				std::printf("pps size: %d type: %d\n", size_, type_);
				payload_.clear();
				payload_.reserve(size_);
				status_ = PPS_PAYLOAD;
			break;
			case PPS_PAYLOAD:
				--size_;
				payload_.push_back(c);
				if(size_ <= 0) {
					std::printf("pss msg: %.*s\n", payload_.length(), payload_.c_str());
					status_ = PPS_SIZE_1;
				}
			break;
			}
PPS_NEXT:
			++i;
PPS_REDO:
			continue;
PPS_FAIL:
			return i;
		}
		return size;
	}
}

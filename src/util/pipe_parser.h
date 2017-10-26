#pragma once

namespace util {
	class pipe_parser {
	public:
		pipe_parser()
		: status_(PPS_SIZE_1) {}
		size_t execute(const char* data, size_t size);
	private:
		uint8_t  type_;
		uint16_t size_;
		std::string payload_;

		int      status_;
		enum     status_t {
			PPS_SIZE_1,
			PPS_SIZE_2,
			PPS_TYPE,
			PPS_RESERVED,
			PPS_PAYLOAD,
		};
	};
}

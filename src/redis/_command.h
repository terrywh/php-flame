
#pragma once

namespace flame {
namespace redis {
	class _command: public _command_base {
	public:
		struct return_value {
			php::array& rv;
			int       size;
			int        idx;
		};
		_command(const php::string& cmd, const php::array& arg, int m = _command_base::REPLY_SIMPLE);
		_command(const php::string& cmd, const php::parameters& arg, int m = _command_base::REPLY_SIMPLE);
		_command(const php::string& cmd, int m = _command_base::REPLY_SIMPLE);
		std::size_t writer() override;
		std::size_t reader(const char* data, std::size_t size) override;
	private:
		php::string             cmd_;
		php::array              arg_;
		php::stream_buffer       sb_;
		int                  status_;
		int                   csize_;
		php::string           cache_;

		void put(const php::value& v);
		friend class _transaction;
	};
}
}

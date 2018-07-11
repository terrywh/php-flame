#pragma once

namespace flame {
namespace redis {
	class _client;
	class _command;
	class _transaction: public _command_base {
	public:
		_transaction(int m = _command_base::REPLY_EXEC);
		~_transaction();
		std::size_t writer() override;
		std::size_t reader(const char* data, std::size_t size) override;

		void append(_command* cmd);
	private:
		std::list<_command*> qs_;
		std::list<_command*> qr_;
		friend class _client;
	};
}
}
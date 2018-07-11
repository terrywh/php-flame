#pragma once

namespace flame {
namespace mongodb {
	class _connection_base {
	public:
		// 以下函数应在主线程调用
		_connection_base() {}
		virtual ~_connection_base() {}
		virtual _connection_base& exec(std::function<std::shared_ptr<bson_t> (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error)> wk,
			std::function<void (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> d, std::shared_ptr<bson_error_t> error)> fn) = 0;

		void execute(std::shared_ptr<coroutine> co, const php::object& ref, std::shared_ptr<bson_t> cmd, bool write = false);
	};
}
}
#pragma once

namespace flame{
namespace net {
namespace http {
	class server_request: public php::class_base {
	public:
		// property headers array
		// property query   array
		// property body    mixed
		// 声明为 ZEND_ACC_PRIVATE 禁止创建
		php::value __construct(php::parameters& params) {
			return nullptr;
		}
		void init(server_connection_base* conn);
		// server_request() {

		// }
		// ~server_request() {
			
		// }
	};
}
}
}

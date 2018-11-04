#pragma once

namespace flame {
namespace kafka {
	class _consumer;
	class message: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		message();
		void build_ex(std::shared_ptr<_consumer> cs, rd_kafka_message_t* msg);
		php::value __construct(php::parameters& params); // 私有
		php::value __destruct(php::parameters& params);
		php::value to_json(php::parameters& params);
		php::value to_string(php::parameters& params);
	private:
		std::shared_ptr<_consumer> cs_;
		rd_kafka_message_t*       msg_;
		friend class consumer;
	};
}
}

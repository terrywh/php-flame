#pragma once
#include "../vendor.h"

namespace flame::kafka
{
	class _consumer;
	class message: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		~message();
		void build_ex(rd_kafka_message_t* msg);
		php::value __construct(php::parameters& params); // 私有
		php::value to_json(php::parameters& params);
		php::value to_string(php::parameters& params);
	private:
	  rd_kafka_message_t *msg_ = nullptr;

	  friend class _consumer;
	//   friend class _producer;
	};
}

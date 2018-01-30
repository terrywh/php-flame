#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "db.h"
#include "redis.h"
#include "mongodb/mongodb.h"
#include "mysql/mysql.h"
#include "kafka/kafka.h"
#include "rabbitmq/rabbitmq.h"

namespace flame {
namespace db {
	void init(php::extension_entry& ext) {
		php::class_entry<redis> class_redis("flame\\db\\redis");
		class_redis.add<&redis::connect>("connect");
		class_redis.add<&redis::close>("close");
		class_redis.add<&redis::__call>("__call", {
			php::of_string("name"),
			php::of_array("arg")
		});
		class_redis.add<&redis::hmget>("hmget");
		class_redis.add<&redis::subscribe>("subscribe");
		class_redis.add<&redis::psubscribe>("psubscribe");
		class_redis.add<&redis::stop_all>("stop_all");
		class_redis.add<&redis::quit>("quit");
		ext.add(std::move(class_redis));
		// 子模块
		mongodb::init(ext);
		mysql::init(ext);
		kafka::init(ext);
		rabbitmq::init(ext);
	}
}
}

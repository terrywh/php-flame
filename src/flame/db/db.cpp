#include "db.h"
#include "redis.h"
#include "mongodb/mongodb.h"
#include "mysql/mysql.h"

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
		class_redis.add<&redis::quit>("quit");
		class_redis.add<&redis::hgetall>("hgetall");
		class_redis.add<&redis::hmget>("hmget");
		class_redis.add<&redis::mget>("mget");
		class_redis.add<&redis::subscribe>("subscribe");
		ext.add(std::move(class_redis));
		// mongodb
		mongodb::init(ext);
		// mysql
		mysql::init(ext);
	}
}
}

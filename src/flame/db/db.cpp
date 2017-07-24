#include "db.h"
#include "redis_client.h"

namespace flame {
namespace db {
	void init(php::extension_entry& ext) {
		php::class_entry<redis_client> class_redis_client("flame\\db\\redis_client");
		class_redis_client.add<&redis_client::__construct>("__construct");
		class_redis_client.add<&redis_client::__call>("__call", {
			php::of_string("name"),
			php::of_array("arg")
		});
		class_redis_client.add<&redis_client::connect>("connect");
		class_redis_client.add<&redis_client::close>("close");
		class_redis_client.add<&redis_client::quit>("quit");
		class_redis_client.add<&redis_client::getlasterror>("getlasterror");
		class_redis_client.add<&redis_client::hgetall>("hgetall");
		class_redis_client.add<&redis_client::hmget>("hmget");
		class_redis_client.add<&redis_client::subscribe>("subscribe");
		ext.add(std::move(class_redis_client));	
	}
}
}

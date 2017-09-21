#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class collection: public php::class_base {
	public:
		~collection();
		php::value __debugInfo(php::parameters& params);
		php::value count(php::parameters& params);
	private:
		php::object          client_;
		mongoc_collection_t* collection_ = nullptr;
		php::value           result_;
		inline void init(const php::object& client, mongoc_collection_t* collection) {
			client_ = client;
			collection_ = collection;
		}
		static void count_work(flame::fiber* fib, void* data);
		static void count_done(flame::fiber* fib, void* data);

		friend class client;
	};
}
}
}

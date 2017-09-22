#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class collection: public php::class_base {
	public:
		~collection();
		php::value __debugInfo(php::parameters& params);
		php::value count(php::parameters& params);
		php::value insert_one(php::parameters& params);
		php::value insert_many(php::parameters& params);
	private:
		php::object          client_;
		mongoc_collection_t* collection_ = nullptr;
		inline void init(const php::object& client, mongoc_collection_t* collection) {
			client_ = client;
			collection_ = collection;
		}
		static void default_cb(uv_work_t* w, int status);
		static void count_wk(uv_work_t* w);
		static void insert_one_wk(uv_work_t* w);
		static void insert_many_wk(uv_work_t* w);
		friend class client;
	};
}
}
}

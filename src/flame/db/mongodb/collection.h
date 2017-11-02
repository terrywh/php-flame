#pragma once

namespace flame {
namespace db {
namespace mongodb {
	class collection: public php::class_base {
	public:
		collection();
		~collection();
		php::value __debugInfo(php::parameters& params);
		php::value count(php::parameters& params);
		php::value insert_one(php::parameters& params);
		php::value insert_many(php::parameters& params);
		php::value remove_one(php::parameters& params);
		php::value remove_many(php::parameters& params);
		php::value update_one(php::parameters& params);
		php::value update_many(php::parameters& params);
		php::value find_one(php::parameters& params);
		php::value find_many(php::parameters& params);
	private:
		php::object          client_object;
		mongoc_client_t*     client_;
		mongoc_collection_t* collection_;
		void init(const php::object& client_object,
			mongoc_client_t* client, mongoc_collection_t* collection);
		static void default_cb(uv_work_t* w, int status);
		static void count_wk(uv_work_t* w);
		static void insert_one_wk(uv_work_t* w);
		static void insert_many_wk(uv_work_t* w);
		static void remove_one_wk(uv_work_t* w);
		static void remove_many_wk(uv_work_t* w);
		static void update_wk(uv_work_t* w);
		static void find_one_wk(uv_work_t* w);
		static void find_many_wk(uv_work_t* w);
		friend class client;
	};
}
}
}

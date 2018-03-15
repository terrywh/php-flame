#pragma once

namespace flame {
namespace db {
namespace mongodb {
	void init(php::extension_entry& ext);
	void fill_with(bson_t* doc, const php::array& arr);
	void fill_with(php::array& arr, const bson_t* doc);
	php::value from(bson_iter_t* iter);

	class stack_bson_t {
	public:
		stack_bson_t();
		stack_bson_t(const php::array& arr);
		~stack_bson_t();
		void fill_with(const php::array& arr);
		operator bson_t*() const;
	private:
		bson_t doc_;
	};
}
}
}

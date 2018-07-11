#pragma once

namespace flame {
namespace mongodb {
	class date_time: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value to_string(php::parameters& params);
		php::value unix(php::parameters& params);
		php::value unix_ms(php::parameters& params);
		php::value to_datetime(php::parameters& params);
		php::value to_json(php::parameters& params);
	private:
		std::int64_t tm_;
		friend void append_object(bson_t* doc, const php::string& key, const php::object& o);
		friend php::value convert(bson_iter_t* i);
	};
}
}
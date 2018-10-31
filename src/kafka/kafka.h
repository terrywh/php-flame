#pragma once

namespace flame {
namespace kafka {
	void declare(php::extension_entry& ext);
	php::value consume(php::parameters& params);
    php::value produce(php::parameters& params);

	php::array convert(rd_kafka_headers_t* headers);
    rd_kafka_headers_t* convert(php::array headers);
}
}
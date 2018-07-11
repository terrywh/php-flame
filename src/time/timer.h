#pragma once

namespace flame {
namespace time {
	class timer: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		timer();
		php::value __construct(php::parameters& params);
		php::value start(php::parameters& params);
		php::value close(php::parameters& params);
	private:
		boost::asio::steady_timer  tm_;
		php::callable              cb_;
		std::shared_ptr<coroutine> co_;

		void start_ex(bool once = false);
	};
}
}
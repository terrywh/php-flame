#pragma once
#include "future.h"

namespace flame {
	class asyncfn {
	public:
		typedef future<asyncfn> future_t;
		asyncfn();
		enum {
			REJECTED = -1,
			PENDING  =  0,
			RESOLVED =  1,
		};
	protected:
		php::object    future_;
		php::value     value_;
	private:
		int            stats_;
		php::generator gen_;

		void set_continue(const php::generator& gen);
		void try_continue();


		uv_work_t   work_;
		php::callable cb_;
		static void work_cb(uv_work_t* work_);
		static void after_work_cb(uv_work_t* work_, int status);

		friend php::value async(php::parameters& params);
		friend void coroutine(php::generator gen);
	};
	php::value async(php::parameters& params);
}
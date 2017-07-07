#pragma once
#include "future.h"

namespace flame {
	class promise {
	public:
		typedef future<promise> future_t;
		promise();
		
		void resolve();
		void resolve(const php::value& v);
		void reject(const php::exception& e);
		void reject(const php::value& e);
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

		friend void coroutine(php::generator gen);
	};
}

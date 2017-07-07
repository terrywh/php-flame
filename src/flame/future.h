#pragma once

namespace flame {
	template <class T>
	class future: public php::class_base {
	private:
		T*     t;
		friend T;
		friend void coroutine(php::generator gen);
	};
}
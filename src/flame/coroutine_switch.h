#pragma once

namespace flame {
class coroutine_switch {
public:
	coroutine_switch(coroutine* target)
	: origin_(coroutine::current) {
		coroutine::current = target;
	}
	~coroutine_switch() {
		coroutine::current = origin_;
	}
private:
	coroutine* origin_;
};
}
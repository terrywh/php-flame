#include "promise.h"
#include "flame.h"

namespace flame {
	promise::promise()
	: future_(php::object::create<future_t>())
	, stats_(PENDING)
	, value_(nullptr) {
		future_t* pf = future_.native<future_t>();
		pf->t = this;
	}
	void promise::resolve() {
		stats_ = RESOLVED;
		// value_ = nullptr;
		try_continue();
	}
	void promise::resolve(const php::value& v) {
		stats_ = RESOLVED;
		value_ = v;
		try_continue();
	}
	void promise::reject(const php::value& e) {
		stats_ = REJECTED;
		value_ = e;
		try_continue();
	}
	void promise::reject(const php::exception& e) {
		stats_ = REJECTED;
		value_ = e;
		try_continue();
	}
	void promise::set_continue(const php::generator& gen) {
		gen_ = (const php::value&)gen;
		try_continue(); // 允许提前返回
	}
	void promise::try_continue() {
		if(stats_ == PENDING || gen_.is_empty()) return;
		
		if(stats_ == RESOLVED) {
			gen_.send(value_);
		}else{
			gen_.throw_exception(value_);
		}
		// 协程继续
		coroutine(gen_);
	}
	
}
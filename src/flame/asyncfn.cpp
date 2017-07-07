#include "asyncfn.h"
#include "flame.h"

namespace flame {
	asyncfn::asyncfn()
	: future_(php::object::create<future_t>())
	, stats_(PENDING)
	, value_(nullptr) {
		future_t* pf = future_.native<future_t>();
		pf->t = this;
		work_.data = this;
	}

	void asyncfn::work_cb(uv_work_t* work_) {
		asyncfn* af = reinterpret_cast<asyncfn*>(work_->data);
		zval* cb = reinterpret_cast<php::value&>(af->cb_);
		zval* fb = reinterpret_cast<php::value&>(af->future_);
		zend_fcall_info_cache fcc = empty_fcall_info_cache;
		zend_fcall_info fci = empty_fcall_info;

		zval oo;
		object_init_ex(&oo, php::class_entry<php::class_closure>::entry());

		fci.size = sizeof(zend_fcall_info);
		fci.retval = af->value_;
		fci.object = Z_OBJ(oo);
		fci.no_separation = 1;
		fcc.initialized = 1;
		fcc.object = Z_OBJ(oo);
		fcc.calling_scope = Z_OBJ(oo)->ce;
		fcc.called_scope = Z_OBJ(oo)->ce;
		fcc.function_handler = (zend_function*)zend_get_closure_method_def(cb);
		zend_call_function(&fci, &fcc);

	}
	void asyncfn::after_work_cb(uv_work_t* work_, int status) {
		asyncfn* af = reinterpret_cast<asyncfn*>(work_->data);
		af->stats_ = RESOLVED;
		af->try_continue();
		delete af;
	}

	void asyncfn::set_continue(const php::generator& gen) {
		gen_ = (const php::value&)gen;
		
		uv_queue_work(uv_default_loop(), &work_, work_cb, after_work_cb);
	}
	void asyncfn::try_continue() {
		if(stats_ == RESOLVED) {
			gen_.send(value_);
		}else{
			gen_.throw_exception(value_);
		}
		// 协程继续
		coroutine(gen_);
	}

	php::value async(php::parameters& params) {
		asyncfn* af = new asyncfn();
		af->cb_ = params[0];
		// !!! 这里并没有将 work 指派，推迟到 set_continue 中，
		// 这样能够防止在主线程 yield 与工作线程 work 之间出现竞争
		return af->future_;
	}
}
	
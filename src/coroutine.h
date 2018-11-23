#pragma once
#include "vendor.h"

namespace flame {
	class coroutine : public std::enable_shared_from_this<coroutine>
	{
	public:
		struct php_context_t
		{
			zend_vm_stack vm_stack;
			zval *vm_stack_top;
			zval *vm_stack_end;
			zend_class_entry *scope;
			zend_execute_data *current_execute_data;
		};
		static coroutine::php_context_t   php_context;
		// 当前协程
		static coroutine* current;
		static void save_context(php_context_t &ctx);
		static void restore_context(php_context_t& ctx);
		static void start(php::callable fn, zend_execute_data* execute_data = nullptr);
		coroutine(php::callable&& fn);

		void suspend();
		void resume();
		php::callable fn_;
		// boost::context::continuation c1_;
		// boost::context::continuation c2_;
		boost::context::fiber c1_;
		boost::context::fiber c2_;
		php_context_t php_;
	};

	struct coroutine_handler
	{
	public:
		// coroutine_handler(std::shared_ptr<coroutine> co)
		coroutine_handler(coroutine *co);
		~coroutine_handler();
		void operator()(const boost::system::error_code& e, std::size_t n = 0);
		void resume();
		void suspend();
		boost::system::error_code error;
		std::size_t 	          nsize;
		coroutine *co_;
	private:
		
	};
} // namespace flame

namespace boost::asio
{

	template <>
	class async_result<::flame::coroutine_handler>
	{
	public:
		explicit async_result(::flame::coroutine_handler& ch) : ch_(ch) {
			
		}
		using type = void;
		void get()
		{
			ch_.suspend();
		}
	private:
		::flame::coroutine_handler &ch_;
	};
} // namespace boost::asio
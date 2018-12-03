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
		//
		static boost::context::fixedsize_stack stack_allocator;
		static std::size_t count;
		// 当前协程
		static std::shared_ptr<coroutine> current;
		static void save_context(php_context_t &ctx);
		static void restore_context(php_context_t& ctx);
		static php_context_t global_context;
		static std::shared_ptr<coroutine> start(php::callable fn);
		coroutine(php::callable fn);
		~coroutine();

		void suspend();
		void resume();
		php::callable fn_;
		// boost::context::continuation c1_;
		// boost::context::continuation c2_;
		boost::context::fiber c1_;
		boost::context::fiber c2_;
		php_context_t php_;
		// 用于部分异步请求返回长度信息
		std::size_t len_;
		
	};

	struct coroutine_handler
	{
	public:
		coroutine_handler();
		coroutine_handler(std::shared_ptr<coroutine> co);
		~coroutine_handler();
		// !!! 慎用: 目前仅用于空构造后指定协程
		void reset(std::shared_ptr<coroutine> co);
		void reset();
		operator bool() const;
		void operator()(const boost::system::error_code& e, std::size_t n = 0);
		coroutine_handler& operator [](boost::system::error_code& e);
		void resume();
		void suspend();
		boost::system::error_code* er_;
		std::shared_ptr<coroutine> co_;
        std::shared_ptr<std::atomic<int>> stat_;

		friend bool operator<(const coroutine_handler &ch1, const coroutine_handler &ch2);

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
		using type = std::size_t;
		type get()
		{
			ch_.suspend();
			return ch_.co_->len_;
		}
	private:
		::flame::coroutine_handler &ch_;
	};
} // namespace boost::asio

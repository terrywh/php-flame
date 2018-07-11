#pragma once

namespace flame {
	class coroutine;
	class dynamic_buffer;
namespace redis {
	// 考虑上层处理
	class _command_base {
	public:
		struct return_value {
			php::array  value;
			php::string field;
			int         index;
			int          size;
		};
		_command_base(int m)
		: m_(m)
		, rt_('\0') {
			rv_.push({nullptr, nullptr, 0, 1}); // 至少期待一个响应
		}
		virtual ~_command_base() {}
		// 返回值处理方式
		enum reply_type {
			REPLY_SIMPLE = 0,
			REPLY_ASSOC_ARRAY_1 = 1, // 返回数据多项按 KEY VAL 生成关联苏族
			REPLY_ASSOC_ARRAY_2 = 2, // 第一层普通数组, 第二层关联数组 (HSCAN/ZSCAN)
			REPLY_COMBINE_1 = 3, // 与参数结合生成关联数组
			REPLY_COMBINE_2 = 4, // 同上, 但偏移错位 1 个参数
			REPLY_EXEC = 0x100, // 事务特殊处理方式（需要结合上面几种方式）
			REPLY_PIPE = 0x200, // 与事务形式相同
		};
		// 多次调用，直到 boost::none 发送完毕
		virtual std::size_t writer() = 0;
		virtual std::size_t reader(const char* data, std::size_t size) = 0;
	protected:
		boost::asio::const_buffer sbuffer_;
		// 返回值处理方式
		int m_;
		// 当前操作协程
		std::shared_ptr<flame::coroutine> co_;
		std::stack<return_value>          rv_; // 返回数据
		char                              rt_; // 返回类型

		friend class _client;
		friend class _transaction;
	};
}
}
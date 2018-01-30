#pragma once

namespace flame {
namespace db {
namespace rabbitmq {
	class table {
	public:
		table();
		table(const php::array& map);
		void assign(const php::array& map);
		// 注意：填充流程需要“引用”PHP对象内存
		void fill(amqp_table_t* t);

		static php::array convert(amqp_table_t* t);
		static php::array convert(amqp_array_t* a);
		static php::value convert(amqp_field_value_t* v);
	private:
		void reset();
		php::array                      sdata_;
		std::vector<amqp_table_entry_t> entry_;
	};


}
}
}

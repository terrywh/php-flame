#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "table.h"

namespace flame {
namespace db {
namespace rabbitmq {
	table::table()
	: sdata_(nullptr)
	, entry_(0) {}
	table::table(const php::array& map)
	: sdata_(map)
	, entry_(map.length()) {
		reset();
	}
	void table::assign(const php::array& map) {
		sdata_ = map;
		reset();
	}
	void table::fill(amqp_table_t* t) {
		if(entry_.size() > 0) {
			t->num_entries = entry_.size();
			t->entries = entry_.data();
		}else{
			*t = amqp_empty_table;
		} 
	}
	void table::reset() {
		int index = 0;
		entry_.resize(sdata_.length());
		for(auto i=sdata_.begin(); i!=sdata_.end(); ++i) {
			php::string  key = i->first.to_string();
			php::value   val = i->second;
			php::string& str = val;
			amqp_table_entry_t& entry = entry_[index++];
			entry.key.len    = key.length();
			entry.key.bytes  = key.data();

			switch(val.type()) {
			case IS_TRUE:
				entry.value.kind = AMQP_FIELD_KIND_BOOLEAN;
				entry.value.value.boolean = val.is_true();
				break;
			case IS_FALSE:
				entry.value.kind = AMQP_FIELD_KIND_BOOLEAN;
				entry.value.value.boolean = !val.is_false();
				break;
			case IS_LONG:
				entry.value.kind = AMQP_FIELD_KIND_I64;
				entry.value.value.i64 = static_cast<int64_t>(val);
			break;
			case IS_DOUBLE:
				entry.value.kind = AMQP_FIELD_KIND_F64;
				entry.value.value.i64 = static_cast<double>(val);
			break;
			case IS_STRING:
				entry.value.kind = AMQP_FIELD_KIND_BYTES;
				entry.value.value.bytes.len   = str.length();
				entry.value.value.bytes.bytes = str.data();
			break;
			default:
				entry.value.kind = AMQP_FIELD_KIND_VOID;
			}
		}
	}
	php::array table::convert(amqp_table_t* t) {
		php::array map(t->num_entries);
		for(int i=0;i<t->num_entries;++i) {
			amqp_table_entry_t* e = t->entries + i;
			map.at( php::string((const char*)e->key.bytes, e->key.len) ) = convert(&e->value);
		}
		return std::move(map);
	}
	php::array table::convert(amqp_array_t* a) {
		php::array map(a->num_entries);
		for(int i=0;i<a->num_entries;++i) {
			map.at( i ) = convert(a->entries + i);
		}
		return std::move(map);
	}
	php::value table::convert(amqp_field_value_t* v) {
		switch(v->kind) {
		case AMQP_FIELD_KIND_BOOLEAN:
			return v->value.boolean ? php::BOOL_YES : php::BOOL_NO;
		case AMQP_FIELD_KIND_I8:
			return static_cast<int>(v->value.i8);
		case AMQP_FIELD_KIND_U8:
			return static_cast<int>(v->value.u8);
		case AMQP_FIELD_KIND_I16:
			return static_cast<int>(v->value.i16);
		case AMQP_FIELD_KIND_U16:
			return static_cast<int>(v->value.u16);
		case AMQP_FIELD_KIND_I32:
			return static_cast<int>(v->value.i32);
		case AMQP_FIELD_KIND_U32:
			return static_cast<int64_t>(v->value.u32);
		case AMQP_FIELD_KIND_I64:
			return static_cast<int64_t>(v->value.i64);
		case AMQP_FIELD_KIND_U64:
		case AMQP_FIELD_KIND_TIMESTAMP:
			return static_cast<int64_t>(v->value.u64);
		case AMQP_FIELD_KIND_F32:
			return static_cast<double>(v->value.f32);
		case AMQP_FIELD_KIND_F64:
			return v->value.f64;
		case AMQP_FIELD_KIND_UTF8:
		case AMQP_FIELD_KIND_BYTES:
			return php::string((const char*)v->value.bytes.bytes, v->value.bytes.len);
		case AMQP_FIELD_KIND_TABLE:
			return convert(&v->value.table);
		case AMQP_FIELD_KIND_ARRAY:
			return convert(&v->value.array);
		case AMQP_FIELD_KIND_DECIMAL:
			return nullptr; // 不支持
		}
	}
}
}
}
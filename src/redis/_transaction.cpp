#include "../coroutine.h"
#include "_command_base.h"
#include "_command.h"
#include "_client.h"
#include "_transaction.h"

namespace flame {
namespace redis {
	_transaction::_transaction(int m)
	: _command_base(m) {
		if(m_ & _command_base::REPLY_EXEC) {
			qs_.push_back(new _command("MULTI", _command_base::REPLY_SIMPLE));
		}else/*if(m & _command_base::REPLY_PIPE)*/ {
			rv_.top().value = php::array(4);
		}
	}
	_transaction::~_transaction() {
		while(!qs_.empty()) {
			delete qs_.front();
			qs_.pop_front();
		}
		while(!qr_.empty()) {
			delete qr_.front();
			qr_.pop_front();
		}
	}
	std::size_t _transaction::writer() {
		while(!qs_.empty()) {
			std::size_t size = 0;
			while((size = qs_.front()->writer()) > 0) {
				sbuffer_ = qs_.front()->sbuffer_;
				return size;
			}
			qr_.splice(qr_.end(), qs_, qs_.begin());
		}
		return 0;
	}
	std::size_t _transaction::reader(const char* data, std::size_t size) {
		std::size_t r = 0;
		while(!qr_.empty()) {
			while(qr_.front()->rv_.size() > 1 || qr_.front()->rv_.top().size > 0) {
				if(r > 0) return r;
				// 由于每次调用 reader 是需要读到新的数据的, 这里有可能出现, 已经全部接收完成的情况
				// 故, 不能直接 return 需要再次确认结束条件
				r = qr_.front()->reader(data, size);
			}
			rt_ = qr_.front()->rt_;
			if(m_ & _command_base::REPLY_PIPE) {
				rv_.top().value.set(rv_.top().value.size(), qr_.front()->rv_.top().value);
			}else if(qr_.size() == 1/* && (m_ & _command_base::REPLY_EXEC)*/) {
				rv_.swap(qr_.front()->rv_);
			}
			delete qr_.front();
			qr_.pop_front();
			if(rt_ == '-' && (m_ & _command_base::REPLY_EXEC)) break;
		}
		rv_.top().size = 0;
		if(rt_ != '-') {
			rt_ = '*';
		}
		return r;
	}
	void _transaction::append(_command* cmd) {
		cmd->m_ |= m_;
		qs_.push_back(cmd);
	}
}
}
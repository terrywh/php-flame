#include "_command_base.h"
#include "_command.h"

namespace flame {
namespace redis {

	_command::_command(const php::string& cmd, const php::array& arg, int m)
	: _command_base(m)
	, cmd_(cmd)
	, arg_(arg.size())
	, status_(-1) {
		for(int i=0; i<arg.size(); ++i) {
			arg_[i] = arg[i].to_string();
		}
	}
	_command::_command(const php::string& cmd, const php::parameters& arg, int m)
	: _command_base(m)
	, cmd_(cmd)
	, arg_(arg.size())
	, status_(-1) {
		for(int i=0; i<arg.size(); ++i) {
			arg_[i] = arg[i].to_string();
		}
	}
	_command::_command(const php::string& cmd, int m)
	: _command_base(m)
	, cmd_(cmd)
	, status_(-1) {
		arg_ = php::array(0);
	}
	// 优化的发送方式 (较大的 BUFFER 不复制直接发送, 小 BUFFER 拼接复制一起发送)
	std::size_t _command::writer() {
		// 清理上次已经发送的数据
		sb_.consume(sb_.size());
		if(status_ >= (int)arg_.size() * 3) {
			return 0;
		}
		std::ostream os(&sb_);
		if(status_ == -1) { // 命令
			os << "*" << std::to_string(1 + arg_.size()) << "\r\n$" << cmd_.size() << "\r\n" << cmd_ << "\r\n";
			++status_;
		}
		while(status_ < arg_.size() * 3) {
			// 从 1 开始发送对应参数项 (较大的参数项单独发送, 小数据项拼接发送)
			int offset_ = status_ / 3;
			php::string str = arg_[offset_];
			if(status_ % 3 == 0) {
				os << "$" << str.size() << "\r\n";
				++status_;
			}
			if(status_ % 3 == 1) {
				if(str.size() > 256) { // 较大的参数项单独发送(不发生拷贝)
					if(sb_.size() > 0) {
						break;
					} else {
						++status_;
						sbuffer_ = boost::asio::const_buffer(str.data(), str.size());
						return str.size();
					}
				}else{
					os << str;
					++status_;
				}
			}
			if(status_ % 3 == 2) {
				os << "\r\n";
				++status_;
			}
		}
		sbuffer_ = boost::asio::const_buffer(sb_.data(), sb_.size());
		return sb_.size();
	}
	// 上层会每次将 一行 数据交过来解析
	std::size_t _command::reader(const char* data, std::size_t size) {
		if(rt_ == '\0') {
			status_ = 0;
			rt_  = data[0];
		}
		if(status_ == 0) { // 简单类型 + - : 仅有一行
			switch(data[0]) {
			case '-':
			case '+':
				put(php::string(data + 1, size - 3));
				break;
			case ':':
				put(std::strtol(data + 1, nullptr, 10));
				break;
			case '$':
				csize_ = std::strtol(data + 1, nullptr, 10);
				if(csize_ == -1) {
					put(nullptr);
				}else{
					cache_  = php::string(csize_);
					status_ = 1;
				}
				break;
			case '*':
				csize_ = std::strtol(data + 1, nullptr, 10);
				if(csize_ > 0) {
					rv_.push({php::array(csize_), nullptr, 0, csize_});
				}else if(csize_ == 0) {
					put(php::array(0));
				}else{
					put(nullptr);
				}
				break;
			default:
				assert(0 && "未知响应类型");
			}
		}else{ // BULK STRING 的情况
			if(csize_ > size) {
				std::memcpy(cache_.data() + cache_.size() - csize_, data, size);
				csize_ -= size;
			}else{
				std::memcpy(cache_.data() + cache_.size() - csize_, data, csize_);
				csize_ = 0;
				put(cache_);
				status_ = 0;
			}
		}
		return size;
	}

	void _command::put(const php::value& v) {
		if(rv_.top().value.typeof(php::TYPE::ARRAY)) {
			switch(m_) {
			case _command_base::REPLY_ASSOC_ARRAY_2:
				if(rv_.size() == 3) {
					goto AS_REPLY_ASSOC_ARRAY_1;
				}else{
					goto AS_REPLY_SIMPLE; // 在 2 层按普通数组方式操作
				}
			case _command_base::REPLY_SIMPLE:
AS_REPLY_SIMPLE:
				rv_.top().value.set(rv_.top().index++, v);
				break;
			case _command_base::REPLY_ASSOC_ARRAY_1:
AS_REPLY_ASSOC_ARRAY_1:
				if(rv_.top().index++ % 2 == 0) {
					rv_.top().field = v;
				}else{
					rv_.top().value.set(rv_.top().field, v);
				}
				break;
			case _command_base::REPLY_COMBINE_1:
				rv_.top().value.set(php::string(arg_[rv_.top().index ++]), v);
				break;
			case _command_base::REPLY_COMBINE_2:
				rv_.top().value.set(php::string(arg_[++ rv_.top().index]), v);
				break;
			}
		}else{
			rv_.top().value = v;
		}
		// 存在 REPLY_EXEC 标识只接收一个 QUEUED 响应
		if(m_ & _command_base::REPLY_EXEC) {
			// m_ &= ~_command_base::REPLY_EXEC;
			--rv_.top().size;
			assert(rv_.size() == 1 && rv_.top().size == 0);
		}else if(--rv_.top().size == 0 && rv_.size() > 1) {
			php::value v = rv_.top().value;
			rv_.pop();
			put(v);
		}
	}
}
}

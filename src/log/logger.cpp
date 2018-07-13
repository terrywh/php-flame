#include "../controller.h"
#include "../coroutine.h"
#include "../time/time.h"
#include "logger.h"
#include "log.h"

namespace flame {
namespace log {
	std::unique_ptr<boost::interprocess::message_queue> logger::queue_;
	void logger::declare(php::extension_entry& ext) {
		php::class_entry<logger> class_logger("flame\\log\\logger");
		class_logger
			.method<&logger::__construct>("__construct", {
				{"filepath", php::TYPE::STRING, false, true},
			})
			.method<&logger::write>("write", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::info>("info", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::warn>("warn", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::fail>("fail", {
				{"message", php::TYPE::STRING}
			})
			.method<&logger::rotate>("rotate");

		ext.add(std::move(class_logger));
	}
	bool logger::to_file() {
		return fpath_[0] == '/';
	}
	std::string logger::write(boost::format& fmt) {
		std::string data = (fmt % fname_).str();
		assert(data.c_str()[0] == ' ');
		queue_->send(data.c_str(), data.size(), 0);
		return std::move(data);
	}
	php::value logger::write(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os.put(' ');
		os << fname_;
		os.put(' ');
		write_ex(os, params);
		queue_->send(sb.data(), sb.size(), 0);
		return nullptr;
	}
	php::value logger::info(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os.put(' ');
		os << fname_;
		os << " [" << time::datetime() << "] (INFO)";
		write_ex(os, params);
		queue_->send(sb.data(), sb.size(), 0);
		return nullptr;
	}
	php::value logger::warn(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os.put(' ');
		os << fname_;
		os << " [" << time::datetime() << "] (WARN)";
		write_ex(os, params);
		queue_->send(sb.data(), sb.size(), 0);
		return nullptr;
	}
	php::value logger::fail(php::parameters& params) {
		php::stream_buffer sb;
		std::ostream os(&sb);
		os.put(' ');
		os << fname_;
		os << " [" << time::datetime() << "] (FAIL)";
		write_ex(os, params);
		queue_->send(sb.data(), sb.size(), 0);
		return nullptr;
	}
	php::value logger::rotate(php::parameters& params) {
		queue_->send("r", 1, 0); // rotate 信号
		return nullptr;
	}
	void logger::write_ex(std::ostream& os, php::parameters& params) {
		for(int i=0;i<params.size();++i) {
			os << ' ' << params[i].ptr();
		}
	}
	void logger::on_sigusr2(const boost::system::error_code& error, int sig) {
		if(!error) {
			write(boost::format(" %2% [%1%] (INFO) logger rotating ...") % time::datetime());
			queue_->send("r", 1, 0); // rotate 信号
			signal_->async_wait(std::bind(&logger::on_sigusr2, this, std::placeholders::_1, std::placeholders::_2));
		}
	}
	logger::logger()
	: fname_(32, '\0') {
		if(controller_->type == controller::MASTER) {
			queue_.reset();
		}
	}
	logger::~logger() {
		// 主进程的 logger 退出需要通知日志线程退出
		if(controller_->type == controller::MASTER) {
			if(queue_) {
				queue_->send("c", 1, 0);
				writer_.join();
				queue_.reset();
			}
		}
	}
	php::value logger::__construct(php::parameters& params) {
		if(params.size() > 0) {
			fpath_ = params[0].to_string();
		}else{
			fpath_ = controller_->environ["FLAME_PROCESS_TITLE"].to_string();
		}
		php::md5(reinterpret_cast<const unsigned char*>(fpath_.c_str()), fpath_.size(), const_cast<char*>(fname_.data()));
		if(controller_->type == controller::MASTER && !logger_) { // 在主进程首次建立
			// 建立默认的输出目标 (最好先清理，否则可能因为残留数据导致死锁)
			boost::interprocess::message_queue::remove(fname_.c_str());
			queue_.reset(new boost::interprocess::message_queue(
				boost::interprocess::create_only, fname_.c_str(), MESSAGE_MAX_COUNT, MESSAGE_MAX_SIZE
			));
			// 启动消费线程
			writer_ = std::thread(std::bind(&logger::writer, this));
			// 主进程在 SIGUSR2 时重载日志(文件)
			signal_.reset(new boost::asio::signal_set(context));
			signal_->async_wait(std::bind(&logger::on_sigusr2, this, std::placeholders::_1, std::placeholders::_2));
		}else if(controller_->type == controller::WORKER && !logger_) { // 在子进程首次创建
			queue_.reset(new boost::interprocess::message_queue(boost::interprocess::open_only, fname_.c_str()));
		}
		if((controller_->type == controller::MASTER && !logger_) || // 在主进程首次建立
			(controller_->type == controller::WORKER && logger_)) { // 在子进程其他创建

			std::string f("n",1); // 1
			f.append(fname_); // 32
			f.push_back(' '); // 1
			f.append(fpath_);

			queue_->send(f.c_str(), f.size(), 0);
		}
		return nullptr;
	}
	void logger::writer() {
		static char     data[MESSAGE_MAX_SIZE];
		static size_t   size;
		static unsigned sort;
ROTATING:
		for(auto i=file_.begin(); i!= file_.end(); ++i) {
			rotate_ex(i->second);
		}

		while(true) {
			queue_->receive(&data, MESSAGE_MAX_SIZE, size, sort);
			switch(data[0]) {
			case 'n': { // 在子进程建立新的 logger 需要周知主进程创建对应的对象
				std::string fname(data + 1, 32);
				std::pair<std::string, std::shared_ptr<std::ostream>>& file = file_[fname];
				file.first = std::string(data + 1 + 32 + 1, size - 1 - 32 - 1);
				rotate_ex(file);
				break;
			}
			case 'c':
				goto CLOSING; // 退出停止机制
			case 'r':
				goto ROTATING; // 重载日志文件
			case ' ': {
				std::string name(data + 1, 32);
				auto file = file_.find(name);
				if(file != file_.end()) {
					file->second.second->write(data + 1 + 32 + 1, size - 1 - 32 - 1);
					file->second.second->put('\n');
					file->second.second->flush();
				}
				break;
			}
			default:
				;
			}
		}
CLOSING:
		boost::interprocess::message_queue::remove(fname_.c_str());
	}
	void logger::rotate_ex(std::pair<std::string, std::shared_ptr<std::ostream>>& file) {
		if(file.first[0] == '/') {
			file.second.reset(new std::ofstream(file.first, std::ios_base::out | std::ios_base::app));
		}else{
			file.second.reset(&std::clog, boost::null_deleter());
		}
	}
}
}

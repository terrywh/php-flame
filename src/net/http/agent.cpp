#include "../../vendor.h"
#include "agent.h"
#include "../../core.h"

namespace net { namespace http {
	void agent::init(php::extension_entry& extension) {
		extension.on_module_startup([] (php::extension_entry& extension) -> bool {
			agent::default_agent_ = new agent();
			agent::default_agent_->start_sweep();
			return true;
		});
		extension.on_module_shutdown([] (php::extension_entry& extension) -> bool {
			delete agent::default_agent_;
			return true;
		});
		php::class_entry<agent> net_http_agent("flame\\net\\http\\agent");
		net_http_agent.add<&agent::__construct>("__construct");
		extension.add(std::move(net_http_agent));
	}
	agent::agent()
	: resolver_(core::io())
	, timer_(core::io())
	, ttl_(15) {

	}
	agent::~agent() {
		boost::system::error_code err;
		timer_.cancel(err);
	}

	php::value agent::__construct(php::parameters& params) {
		ttl_ = std::chrono::seconds(static_cast<int>(params[0]));
		if(ttl_ > std::chrono::seconds::zero()) {
			start_sweep();
		}
	}

	// 需要被 request 反复调用，获取可用的连接
	void agent::acquire(const std::string& domain, const std::string& port, std::function<void (const boost::system::error_code&, std::shared_ptr<tcp::socket>)> handler) {
		std::forward_list<agent_socket_wrapper>& ctr = socket_[domain + ":" + port];
		if(ctr.empty()) {
			// 解析并建立新连接
			resolver_.async_resolve(tcp::resolver::query {domain, port}, [this, handler] (const boost::system::error_code& err, tcp::resolver::iterator i) {
				if(err) {
					handler(err, nullptr);
					return;
				}
				std::shared_ptr<tcp::socket> s = std::make_shared<tcp::socket>(core::io());
				boost::asio::async_connect(*s, i, [s, handler] (const boost::system::error_code& err, tcp::resolver::iterator i) {
					handler(err, s);
				});
			});
		}else{
			// 复用已有连接
			agent_socket_wrapper& asw = ctr.front();
			handler(boost::system::error_code(), asw.socket_);
			ctr.pop_front();
		}
	}
	// 被 response 调用，复用 socket
	void agent::release(const std::string& domain, const std::string& port, std::shared_ptr<tcp::socket> sock) {
		if(ttl_ > std::chrono::seconds::zero()) {
			socket_[domain + ":" + port].push_front(agent_socket_wrapper {
				std::chrono::steady_clock::now() + ttl_,
				sock,
			});
		}else{ // 不复用
			boost::system::error_code err;
			sock->shutdown(tcp::socket::shutdown_both, err);
			sock->close(err);
		}
	}

	agent* agent::default_agent_;

	void agent::start_sweep() {
		timer_.expires_from_now(ttl_);
		timer_.async_wait([this] (const boost::system::error_code& err) {
			assert(!err);
			std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
			boost::system::error_code error;
			// 遍历所有缓存的 socket 并按逻辑清理
			for(auto i=socket_.begin();i!=socket_.end();++i) {
				for(auto j=i->second.begin(); j!= i->second.end(); ++j) {
					if((*j).expire_ >= now) {
						(*j).socket_->shutdown(tcp::socket::shutdown_both, error);
						(*j).socket_->close(error);
					}
				}
			}
			start_sweep();
		});

	}

}}

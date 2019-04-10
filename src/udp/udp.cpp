#include "../controller.h"
#include "../coroutine.h"
#include "udp.h"
#include "socket.h"
#include "server.h"

namespace flame::udp {

	std::unique_ptr<boost::asio::ip::udp::resolver> resolver_;

	void declare(php::extension_entry& ext) {
		gcontroller->on_init([] (const php::array& options) {
            resolver_.reset(new boost::asio::ip::udp::resolver(gcontroller->context_x));
        })->on_stop([] () {
            resolver_.reset();
        });
		ext
			.function<connect>("flame\\udp\\connect", {
				{"address", php::TYPE::STRING},
			});
		socket::declare(ext);
		server::declare(ext);
	}

	php::value connect(php::parameters& params) {
		php::object obj(php::class_entry<socket>::entry());
        socket *ptr = static_cast<socket *>(php::native(obj));

        std::string str = params[0];
        auto pair = addr2pair(str);
        if(pair.first.empty() || pair.second.empty()) throw php::exception(zend_ce_type_error
            , "Failed to connect UDP socket: illegal address format"
            , -1);

		coroutine_handler ch{coroutine::current};
        boost::system::error_code err;
		boost::asio::ip::udp::resolver::results_type eps;
		resolver_->async_resolve(pair.first, pair.second
            , [&err, &obj, &ptr, &eps, &ch] (const boost::system::error_code& error, boost::asio::ip::udp::resolver::results_type results) { // DNS 地址解析

			if(error) err = error;
			else eps = results;
			ch.resume();
		});
		ch.suspend();
		if(err) throw php::exception(zend_ce_exception
                , (boost::format("Failed to resolve address: %s") % err.message()).str()
                , err.value());

        boost::asio::async_connect(ptr->socket_, eps
            , [&err, &obj, &ptr, &ch](const boost::system::error_code &error, const boost::asio::ip::udp::endpoint &edp) { // 连接

            if (error) err = error;
			else {
            	obj.set("local_address", (boost::format("%s:%d") % ptr->socket_.local_endpoint().address().to_string() % ptr->socket_.local_endpoint().port()).str());
				obj.set("remote_address", (boost::format("%s:%d") % ptr->socket_.remote_endpoint().address().to_string() % ptr->socket_.remote_endpoint().port()).str());
			}
            ch.resume();
        });
        ch.suspend();
        if(err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to connect UDP socket: %s") % err.message()).str()
            , err.value());
		ptr->connected_ = true;
        return std::move(obj);
	}
    
    std::pair<std::string, std::string> addr2pair(const std::string &addr) {
        char *s = const_cast<char*>(addr.data()), *p, *e = s + addr.size();
        for (p = e - 1; p > s; --p) if (*p == ':') break; // 分离 地址与端口
        if (*p != ':') return std::make_pair<std::string, std::string>(std::string(addr), "");
        else return std::make_pair<std::string, std::string>(
                std::string(s, p - s), std::string(p+1, e-p-1));
    }

} // namespace flame::tcp

#include "../controller.h"
#include "../coroutine.h"
#include "../udp/udp.h"
#include "tcp.h"
#include "socket.h"
#include "server.h"

namespace flame::tcp {
	static std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
	void declare(php::extension_entry& ext)
    {
		gcontroller->on_init([] (const php::array& options) {
            resolver_.reset(new boost::asio::ip::tcp::resolver(gcontroller->context_x));
        })->on_stop([] () {
            resolver_.reset();
        });
		ext
			.function<connect>("flame\\tcp\\connect", {
				{"address", php::TYPE::STRING},
			});
		socket::declare(ext);
		server::declare(ext);
	}

    php::value connect(php::parameters& params) {
		php::object obj(php::class_entry<socket>::entry());
        socket *ptr = static_cast<socket *>(php::native(obj));

        std::string str = params[0];
        auto pair = udp::addr2pair(str);
        if (pair.first.empty() || pair.second.empty()) throw php::exception(zend_ce_type_error
        	, "Failed to connect TCP socket: illegal address format"
        	, -1);

		coroutine_handler ch{coroutine::current};
        boost::system::error_code err;
		boost::asio::ip::tcp::resolver::results_type eps;
		// DNS 地址解析
		resolver_->async_resolve(pair.first, pair.second, [&err, &eps, &ch] (const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type results) {
			if (error) err = error;
			else eps = results;
            ch.resume();
		});
		ch.suspend();
		if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to resolve address: %s") % err.message()).str()
            , err.value());

		// 连接
        boost::asio::async_connect(ptr->socket_, eps, [&err, &obj, &ptr, &ch](const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &edp) {
            if (error) err = error;
			else {
	            obj.set("local_address", (boost::format("%s:%d")
	            	% ptr->socket_.local_endpoint().address().to_string()
	            	% ptr->socket_.local_endpoint().port() ).str());
				obj.set("remote_address", (boost::format("%s:%d")
					% ptr->socket_.remote_endpoint().address().to_string()
					% ptr->socket_.remote_endpoint().port() ).str());
			}
            ch.resume();
        });
        ch.suspend();
        if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to connect TCP socket: %s") % err.message()).str()
            , err.value());
        return std::move(obj);
	}
} // namespace flame::tcp

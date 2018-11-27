#include "../controller.h"
#include "../coroutine.h"
#include "tcp.h"
#include "socket.h"
#include "server.h"

namespace flame::tcp {
	static std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
	void declare(php::extension_entry& ext)
    {
		gcontroller->on_init([] ()
        {
            resolver_.reset(new boost::asio::ip::tcp::resolver(gcontroller->context_x));
        })->on_stop([] ()
        {
            resolver_.reset();
        });
		ext
			.function<connect>("flame\\tcp\\connect",
            {
				{"address", php::TYPE::STRING},
			});
		socket::declare(ext);
		server::declare(ext);
	}

    std::pair<std::string, std::string> addr2pair(const std::string &addr)
    {
        char *s = const_cast<char*>(addr.data()), *p, *e = s + addr.size();
        for (p = e - 1; p > s; --p)
        {
            if (*p == ':')
                break; // 分离 地址与端口
        }
        if (*p != ':')
        {
            return std::make_pair<std::string, std::string>(std::string(addr), "");
        }
        else
        {
            return std::make_pair<std::string, std::string>(
                std::string(s, p - s), std::string(p+1, e-p-1));
        }
    }
    php::value connect(php::parameters& params) {
		php::object obj(php::class_entry<socket>::entry());
        socket *ptr = static_cast<socket *>(php::native(obj));

        std::string str = params[0];
        auto pair = addr2pair(str);
        if(pair.second.empty()) {
            throw php::exception(zend_ce_type_error, "failed to connect tcp socket: missing port");
        }

		coroutine_handler ch{coroutine::current};
        boost::system::error_code err;
		// DNS 地址解析
		resolver_->async_resolve(pair.first, pair.second, [&err, &obj, &ptr, &ch] (const boost::system::error_code& error, boost::asio::ip::tcp::resolver::results_type edps) {
			if(error) 
            {
                err = error;
                ch.resume();
                return;
            }
			// 连接
            boost::asio::async_connect(ptr->socket_, edps, [&err, &obj, &ptr, &ch](const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &edp) {
                if (error)
                {
                    err = error;
                    ch.resume();
                    return;
                }
                obj.set("local_address", (boost::format("%s:%d") % ptr->socket_.local_endpoint().address().to_string() % ptr->socket_.local_endpoint().port()).str());
				obj.set("remote_address", (boost::format("%s:%d") % ptr->socket_.remote_endpoint().address().to_string() % ptr->socket_.remote_endpoint().port()).str());
                ch.resume();
            });
		});
        ch.suspend();
        if(err) 
        {
            throw php::exception(zend_ce_exception,
                (boost::format("failed to connect tcp socket: (%1%) %2%") % err.value() % err.message()).str(), err.value());
        }
        return std::move(obj);
	}
} // namespace flame::tcp

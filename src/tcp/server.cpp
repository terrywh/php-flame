#include "../controller.h"
#include "../coroutine.h"
#include "../udp/udp.h"
#include "../time/time.h"
#include "server.h"
#include "tcp.h"
#include "socket.h"

namespace flame::tcp {

    void server::declare(php::extension_entry &ext) {
        php::class_entry<server> class_server("flame\\tcp\\server");
		class_server
			.method<&server::__construct>("__construct", {
				{"bind", php::TYPE::STRING},
			})
			.method<&server::run>("run", {
				{"cb", php::TYPE::CALLABLE},
			})
			.method<&server::close>("close");
		ext.add(std::move(class_server));
    }

    server::server()
    : acceptor_(gcontroller->context_x)
    , socket_(gcontroller->context_x) {

    }

    typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;

    php::value server::__construct(php::parameters &params) {
        closed_ = false;
        std::string str_addr = params[0];
        auto pair = udp::addr2pair(str_addr);
        if (pair.first.empty() || pair.second.empty())
            throw php::exception(zend_ce_error_exception
                , "Failed to bind TCP socket: address malformed"
                , -1);
        boost::asio::ip::address addr = boost::asio::ip::make_address(pair.first);
        addr_.address(addr);
        addr_.port(std::atoi(pair.second.c_str()));

        set("address", str_addr);
        acceptor_.open(addr_.protocol());
        boost::asio::socket_base::reuse_address opt1(true);
        acceptor_.set_option(opt1);
        reuse_port opt2(true);
        acceptor_.set_option(opt2);

        boost::system::error_code err;
        acceptor_.bind(addr_, err);
        if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to bind TCP socket: %s") % err.message()).str()
            , err.value());

        acceptor_.listen(boost::asio::socket_base::max_listen_connections, err);
        if (err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to listen TCP socket: %s") % err.message()).str()
            , err.value());
        return nullptr;
    }

    php::value server::run(php::parameters &params) {
        cb_ = params[0];
        coroutine_handler ch {coroutine::current};
        boost::system::error_code err;
        while(!closed_) {
            acceptor_.async_accept(socket_, ch[err]);
            if (err == boost::asio::error::operation_aborted) break;
            else if (err) throw php::exception(zend_ce_error_exception
                , (boost::format("Failed to accept TCP socket: %s") % err.message()).str()
                , err.value());
            else{
                php::object obj(php::class_entry<socket>::entry());
                socket* ptr = static_cast<socket*>(php::native(obj));
                ptr->socket_ = std::move(socket_);
                obj.set("local_address",
                    (boost::format("%s:%d") % ptr->socket_.local_endpoint().address().to_string() % ptr->socket_.local_endpoint().port()).str());
                obj.set("remote_address",
                    (boost::format("%s:%d") % ptr->socket_.remote_endpoint().address().to_string() % ptr->socket_.remote_endpoint().port()).str());

                coroutine::start(php::callable([obj, cb = cb_] (php::parameters& params) -> php::value {
                    try {
                        cb.call({obj});
                    } catch(const php::exception& ex) {
                        auto ft = gcontroller->cbmap->equal_range("exception");
                        for(auto i=ft.first; i!=ft.second; ++i) i->second.call({ex});

                        php::object obj = ex;
                        std::cerr << "[" << time::iso() << "] (ERROR) Uncaught Exception in TCP handler: "<< obj.call("__toString") << "\n";
                    }
                    return nullptr;
                }));
            }
        }
        cb_ = nullptr;
        return nullptr;
    }
    
    php::value server::close(php::parameters &params) {
        if (!closed_) {
            closed_ = true;
            acceptor_.cancel();
        }
        return nullptr;
    }
}

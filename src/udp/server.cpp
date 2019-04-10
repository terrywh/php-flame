#include "../controller.h"
#include "../coroutine.h"
#include "../coroutine_queue.h"
#include "../time/time.h"
#include "udp.h"
#include "server.h"

namespace flame::udp {
    
    void server::declare(php::extension_entry& ext) {
		php::class_entry<server> class_server("flame\\udp\\server");
		class_server
			.method<&server::__construct>("__construct", {
				{"address", php::TYPE::STRING, false, false},
				{"options", php::TYPE::ARRAY, false, true},
			})
			.method<&server::run>("run", {
				{"length", php::TYPE::CALLABLE, false, false},
			})
            .method<&server::send_to>("send_to", {
				{"data", php::TYPE::STRING, false, false},
				{"address", php::TYPE::STRING, false, false},
			})
			.method<&server::close>("close");
		ext.add(std::move(class_server));
	}
    server::server()
	: socket_(gcontroller->context_x)
    , closed_(false)
    , concurrent_(8)
    , max_(64 * 1024) {

	}

    typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;

    php::value server::__construct(php::parameters& params) {
        auto pair = addr2pair(params[0]);
		boost::system::error_code err;
		boost::asio::ip::address address = boost::asio::ip::make_address(std::string_view(pair.first.data(), pair.first.size()), err);
		if(err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to bind: %s") % err.message()).str()
            , err.value());
		std::uint16_t port = std::stoi(pair.second);
        addr_.address(address);
        addr_.port(port);

        socket_.open(addr_.protocol());
        boost::asio::socket_base::reuse_address opt1(true);
        socket_.set_option(opt1);
        reuse_port opt2(true);
        socket_.set_option(opt2);
		socket_.bind(addr_, err);
		if(err) throw php::exception(zend_ce_exception
            , (boost::format("Failed to bind: %s") % err.message()).str()
            , err.value());

        if(params.size() > 1) {
            php::array options = params[1];
            if(options.exists("concurrent"))
                concurrent_ = std::max(std::min(static_cast<int>(options.get("concurrent")), 256), 1);
            if(options.exists("max"))
                max_ = std::min(std::max(static_cast<int>(options.get("max")) , 1), 64 * 1024);
        }
        return nullptr;
    }

    php::value server::run(php::parameters& params) {
		php::callable cb_ = params[0];

        coroutine_handler ch {coroutine::current};
        coroutine_queue<std::pair<php::string, php::string>> q(128);
        // 启动若干协程, 然后进行"并行>"消费
        int count = concurrent_;
        for (int i = 0; i < concurrent_; ++i) {
            // 启动协程开始消费
            coroutine::start(php::value([&q, &count, &ch, &cb_, i] (php::parameters &params) -> php::value {
                coroutine_handler cch {coroutine::current};
                while(auto x = q.pop(cch)) {
                    try {
                        cb_.call({x->first, x->second});
                    } catch(const php::exception& ex) {
                        php::object obj = ex;
                        std::cerr << "[" << time::iso() << "] (ERROR) Uncaught Exception in UDP handler: " << obj.call("__toString") << "\n";
                        // std::clog << "[" << time::iso() << "] (ERROR) " << ex.what() << std::endl;
                    }
                }
                if(--count == 0) ch.resume();
                return nullptr;
            }));
        }
        // 生产
        std::size_t len = 0;
        boost::system::error_code err;
		boost::asio::ip::udp::endpoint ep;
        php::buffer buffer;
        while(!closed_) {
    		socket_.async_receive_from(boost::asio::buffer(buffer.prepare(max_), max_), ep
                , [&len, &err, &ch] (const boost::system::error_code& error, std::size_t nread) {

                if(error) err = error;
                else len = nread;
                ch.resume();
            });
            ch.suspend();
            // 存在可能被关闭, 直接返回
            if(err == boost::asio::error::operation_aborted) goto CLOSING;
            else if(err) throw php::exception(zend_ce_exception
                , (boost::format("Failed to read TCP socket: %s") % err.message()).str()
                , err.value());
            buffer.commit(len);
            q.push(std::make_pair<php::string, php::string>(php::string(std::move(buffer))
                , (boost::format("%s:%d") % ep.address().to_string(err) % ep.port()).str()), ch);
        }
CLOSING:
        q.close();
        ch.suspend();
        return nullptr;
	}

    php::value server::send_to(php::parameters& params) {
        coroutine_handler ch{coroutine::current};

        boost::system::error_code err;
		php::string data = params[0];
		auto pair = addr2pair(params[1]);
		if(pair.first.empty() || pair.second.empty()) throw php::exception(zend_ce_type_error
            , "Failed to send udp packet: illegal address format"
            , -1);

		boost::asio::ip::udp::resolver::results_type eps;
		resolver_->async_resolve(pair.first, pair.second
            , [&ch, &eps, &err] (const boost::system::error_code& error, boost::asio::ip::udp::resolver::results_type results) {

			if(error) err = error;
			else eps = results;
			ch.resume();
		});
		ch.suspend();
		if (err == boost::asio::error::operation_aborted) return nullptr;
		else if(err) throw php::exception(zend_ce_exception
			, (boost::format("Failed to resolve address: %s") % err.message()).str()
            , err.value());

		// 发送
		int sent = 0;
		for(auto i=eps.begin(); i!=eps.end(); ++i) {
			socket_.async_send_to(boost::asio::buffer(data.c_str(), data.size()), *i, ch[err]);
			if(!err) return nullptr;
		}

		throw php::exception(zend_ce_exception
			, (boost::format("Failed to send UDP packet: %s") % err.message()).str()
            , err.value());
	}

    php::value server::close(php::parameters& params) {
        closed_ = true;
        boost::system::error_code err;
        socket_.close(err);
        return true;
    }
}

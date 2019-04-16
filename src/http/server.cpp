#include "../controller.h"
#include "../coroutine.h"
#include "../udp/udp.h"
#include "server.h"
#include "_handler.h"

namespace flame::http {
    
    void server::declare(php::extension_entry &ext)  {
        php::class_entry<server> class_server("flame\\http\\server");
        class_server
            .property({"address", "127.0.0.1:7678"})
            .method<&server::__construct>("__construct", {
                {"address", php::TYPE::STRING},
            })
            .method<&server::before>("before", {
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::after>("after", {
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::put>("put", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::delete_>("delete", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::post>("post", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::patch>("patch", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::get>("get", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::head>("head", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::options>("options", {
                {"path", php::TYPE::STRING},
                {"callback", php::TYPE::CALLABLE},
            })
            .method<&server::run>("run")
            .method<&server::close>("close");
        ext.add(std::move(class_server));
    }

    server::server()
    : accp_(gcontroller->context_x)
    , sock_(gcontroller->context_x)
    , closed_(false) {

    }

    typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;
    php::value server::__construct(php::parameters &params) {
        php::string str = params[0];
        auto pair = udp::addr2pair(str);
        if (pair.first.empty() || pair.second.empty())
            throw php::exception(zend_ce_error_exception
                , "Failed to bind tcp socket: address malformed"
                , -1);
        boost::asio::ip::address addr = boost::asio::ip::make_address(pair.first);
        addr_.address(addr);
        addr_.port(std::atoi(pair.second.c_str()));

        accp_.open(addr_.protocol());
        boost::asio::socket_base::reuse_address opt1(true);
        accp_.set_option(opt1);
        reuse_port opt2(true);
        accp_.set_option(opt2);

        set("address", params[0]);
        return nullptr;
    }

    php::value server::before(php::parameters &params) {
        cb_["before"] = params[0];
        return this;
    }

    php::value server::after(php::parameters &params) {
        cb_["after"] = params[0];
        return this;
    }

    php::value server::put(php::parameters &params) {
        cb_[std::string("PUT:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::delete_(php::parameters &params) {
        cb_[std::string("DELETE:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::post(php::parameters &params) {
        cb_[std::string("POST:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::get(php::parameters &params) {
        cb_[std::string("GET:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::head(php::parameters &params) {
        cb_[std::string("HEAD:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::options(php::parameters &params) {
        cb_[std::string("OPTIONS:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::patch(php::parameters &params) {
        cb_[std::string("PATCH:") + params[0].to_string()] = params[1];
        return this;
    }

    php::value server::run(php::parameters &params) {
        boost::system::error_code err;
        accp_.bind(addr_, err);
        if (err) throw php::exception(zend_ce_error_exception
            , (boost::format("Failed to bind TCP socket: %s") % err.message()).str()
            , err.value());
        accp_.listen(boost::asio::socket_base::max_listen_connections);

        coroutine_handler ch{coroutine::current};
        while(!closed_) {
            boost::system::error_code err;
            accp_.async_accept(sock_, ch[err]);
            if (err == boost::asio::error::operation_aborted) break; 
            else if (err) throw php::exception(zend_ce_error_exception
                , (boost::format("Failed to accept connection: %s") % err.message()).str()
                , err.value());
            else std::make_shared<_handler>(this, std::move(sock_))->start();
        }
        return nullptr;
    }

    php::value server::close(php::parameters &params) {
        if (closed_) return nullptr;
        closed_ = true;
        accp_.cancel();
        accp_.close();
        return nullptr;
    }
} // namespace flame::http

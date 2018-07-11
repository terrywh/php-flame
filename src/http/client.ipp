// !!! include in client.h

namespace flame {
namespace http {
    
    template <typename Stream, typename Handler>
    void client::acquire(std::shared_ptr<php::url> url, std::shared_ptr<Stream>& ptr, Handler && handler)  {
        // TODO 连接管理机制
        create(url, ptr, handler);
    }
    // 创建新连接
    template <typename Handler>
    void client::create(std::shared_ptr<php::url> url, std::shared_ptr<tcp::socket>& ptr, Handler && handler) {
        ptr.reset(new tcp::socket(context));
        resolver_.async_resolve(url->host, std::to_string(url->port), [ptr, handler] (const boost::system::error_code& error, tcp::resolver::results_type eps) {
            if(error) {
                handler(error);
                return;
            }
            boost::asio::async_connect(*ptr, eps, [handler] (const boost::system::error_code& error, const tcp::endpoint& ep) {
                handler(error);
            });
        });
    }
    template <typename Handler>
    void client::create(std::shared_ptr<php::url> url, std::shared_ptr<ssl::stream<tcp::socket>>& ptr, Handler && handler) {
        ptr.reset(new ssl::stream<tcp::socket>(context, context_));
        resolver_.async_resolve(url->host, std::to_string(url->port), [ptr, handler] (const boost::system::error_code& error, tcp::resolver::results_type eps) {
            if(error) {
                handler(error);
                return;
            }
            boost::asio::async_connect(ptr->lowest_layer(), eps, [ptr, handler] (const boost::system::error_code& error, const tcp::endpoint& ep) {
                if(error) {
                    handler(error);
                    return;
                }
                ptr->async_handshake(ssl::stream<tcp::socket>::handshake_type::client, handler);
            });
        });
    }

}
}
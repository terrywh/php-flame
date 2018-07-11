#pragma once

namespace flame {
namespace http {
    class value_body_reader;
    class value_body_writer;
    template <bool isRequest>
    class value_body {
    public:
        // Body
        using value_type = php::value;
        static std::uint64_t size(const value_type& v);
        // BodyReader (Body)
        using reader = value_body_reader;
        // BodyWriter (Body)
        using writer = value_body_writer;
    };
    class value_body_reader {
    public:
        template <bool isRequest, class Fields>
        value_body_reader(boost::beast::http::header<isRequest, Fields>& h, php::value& v);
        void init(boost::optional<std::uint64_t> n, boost::system::error_code& error);
        template <typename ConstBufferSequence>
        std::size_t put(ConstBufferSequence s, boost::system::error_code& error);
        void finish(boost::system::error_code& error);
    private:
        php::value& v_;
        php::stream_buffer b_;
    };
    class value_body_writer {
    public:
        using const_buffers_type = boost::asio::const_buffer;
        template <bool isRequest, class Fields>
        value_body_writer(const boost::beast::http::header<isRequest, Fields>& h, const php::value& v);
        void init(boost::system::error_code& error);
        boost::optional<std::pair<const_buffers_type, bool>> get(boost::system::error_code& error);
    private:
        // const boost::beast::http::header<true>& h_;
        const php::value& v_;
    };
    


} // http
} // flame 
#include "value_body.ipp"

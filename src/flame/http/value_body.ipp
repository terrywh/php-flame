#pragma once

namespace flame::http {
    
    template <bool isRequest, class Fields>
    value_body_reader::value_body_reader(boost::beast::http::header<isRequest, Fields>& h, php::value& v)
    : v_(v) {

    }

    template <typename ConstBufferSequence>
    std::size_t value_body_reader::put(ConstBufferSequence s, boost::system::error_code& error) {
        std::size_t n = 0;
        for (auto i=boost::asio::buffer_sequence_begin(s); i!=boost::asio::buffer_sequence_end(s); ++i) {
            boost::asio::const_buffer cbuf(*i);
            b_.append((const char *)cbuf.data(), cbuf.size());
            n += cbuf.size();
        }
        error.assign(0, error.category());
        return n;
    }

    template <bool isRequest, class Fields>
    value_body_writer::value_body_writer(const boost::beast::http::header<isRequest, Fields>& h, const php::value& v)
    : v_(v) {}

    template <bool isRequest>
    std::uint64_t value_body<isRequest>::size(const value_body<isRequest>::value_type& v) {
        return v.size();
    }

} // namespace flame::http

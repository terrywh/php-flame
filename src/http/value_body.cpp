#include "value_body.h"

namespace flame {
namespace http {
    void value_body_reader::init(boost::optional<std::uint64_t> n, boost::system::error_code& error) {
        error.assign(0, error.category());
    }
    void value_body_reader::finish(boost::system::error_code& error) {
        v_ = std::move(b_);
        error.assign(0, error.category());
    }
    void value_body_writer::init(boost::system::error_code& error) {
        error.assign(0, error.category());
    }
    boost::optional<std::pair<value_body_writer::const_buffers_type, bool>> value_body_writer::get(boost::system::error_code& error) {
        php::string data = v_;
        error.assign(0, error.category());
        return {{boost::asio::const_buffer(data.data(), data.size()), false}};
    }
} // http
} // flame 
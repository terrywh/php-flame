/********************************************************************
 * php-flame - PHP full stack development framework
 * copyright (c) 2020 terrywh
 ********************************************************************/

#ifndef PHP_FLAME_CORE_H
#define PHP_FLAME_CORE_H

#include <boost/asio/io_context.hpp>
#define PHP_FLAME_CORE_VERSION "0.18.0" // 模块版本

namespace flame {
namespace core {
    // PHP 主线程 IO 上下文
    boost::asio::io_context& master_context();
    // PHP 辅线程 IO 上下文
    boost::asio::io_context& worker_context();

}
}

#endif // PHP_FLAME_H
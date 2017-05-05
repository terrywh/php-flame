#pragma once

#include <cstring>
#include <cctype>
#include <cstdint>
#include <cerrno>
#include <memory>
#include <thread>
#include <vector>
// #include <forward_list>

#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

#include <phpext.h>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
using boost::asio::ip::tcp;
using boost::asio::ip::udp;
using boost::asio::ip::address;
#include <boost/lockfree/queue.hpp>

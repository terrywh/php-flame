#pragma once

#include <cstring>
#include <cctype>
#include <cstdint>
#include <cerrno>
#include <memory>
#include <thread>
#include <vector>
#include <map>
#include <forward_list>
#include <tuple>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/signalfd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

#include <phpext.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/http.h>
// #include <boost/asio.hpp>
// #include <boost/asio/steady_timer.hpp>
// using boost::asio::ip::tcp;
// using boost::asio::ip::udp;
// using boost::asio::ip::address;
// #include <boost/lockfree/queue.hpp>
// #include <boost/algorithm/string.hpp>
// #include <boost/logic/tribool.hpp>
// using namespace boost::logic;
#include <boost/format.hpp>

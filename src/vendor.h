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
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <phpext.h>
#include <event2/event_struct.h>
#include <event2/keyvalq_struct.h>
#include "../vendor/queue.h" // 来自 libevent/compat/sys/queue.h
#include <event2/thread.h>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <boost/format.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <lmdb.h>
#include <zlib.h>

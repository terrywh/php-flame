#include "ipc.h"


// coroutine::start(php::callable([this] (php::parameters& params) -> php::value {
//         coroutine_handler ch { coroutine::current };
//         boost::system::error_code ec;
// RECONNECT:
//         socket_.async_connect(address_, ch[ec]);
//         if(ec) {
//             co_sleep(std::chrono::milliseconds(std::rand() % 500 + 500), ch);
//             goto RECONNECT;
//         }
        
//         return nullptr;
//     }));
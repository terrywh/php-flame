#include "time.h"
#include "../controller.h"
#include "../coroutine.h"

namespace flame::time {


static php::value sleep(php::parameters& params) {
    coroutine_handler yield {coroutine::current};
    boost::asio::steady_timer tm(gcontroller->context_x);
    int ms = static_cast<int>(params[0]);
    // std::cout << "sleep: " << ms << std::endl;
    tm.expires_from_now(std::chrono::milliseconds(ms));
    tm.async_wait(yield);
    return nullptr;
}

void declare(php::extension_entry& ext) {
    ext.function<sleep>("flame\\time\\sleep", {
        {"duration", php::TYPE::INTEGER},
    });
}

}
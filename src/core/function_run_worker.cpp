#include "function_run_worker.h"
#include "context.hpp"

#include <algorithm>

namespace flame {
namespace core {

    function_run_worker::function_run_worker()
    : guard_($->io_w.get_executor()) {
        start();
    }

    function_run_worker::~function_run_worker() {
        if($->io_w.stopped()) return;
        // 对象销毁过程进行“正常停止”
        guard_.reset();
        for(auto& worker: worker_) {
            worker.join();
        }
    }

    void function_run_worker::start() {
        auto count = std::max(std::thread::hardware_concurrency()/8, 2u);
        for(int i=0;i<count;++i) {
            worker_.push_back(std::move(std::thread([] () {
                $->io_w.run();
            })));
        }
    }

    void function_run_worker::halt() {
        if($->io_w.stopped()) return;
        // “强制停止”
        guard_.reset();
        $->io_w.stop();
        for(auto& worker: worker_) {
            worker.join();
        }
    }
}
}
#include "worker.h"
#include "core.h"

#include <algorithm>

namespace flame {
namespace core {

    worker::worker()
    : guard_(worker_context().get_executor()) {
        
    }

    void worker::run_ex() {
        worker_context().run();
    }

    void worker::start() {
        auto worker_count = std::max(std::thread::hardware_concurrency()/8, 2u);
        for(int i=0;i<worker_count;++i) {
            worker_.push_back(std::move(std::thread(worker::run_ex)));
        }
    }

    void worker::stop() {
        guard_.reset();
        for(auto& worker: worker_) {
            worker.join();
        }
    }

    void worker::halt() {
        guard_.reset();
        worker_context().stop();
        for(auto& worker: worker_) {
            worker.join();
        }
    }
}
}
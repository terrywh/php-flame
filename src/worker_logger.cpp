#include "worker_logger.h"
#include "worker_logger_manager.h"
#include "worker_logger_buffer.h"

worker_logger::worker_logger(worker_logger_manager* mgr, unsigned int idx)
: idx_(idx)
, wlb_(new worker_logger_buffer(mgr))
, oss_(wlb_.get()) {
    
}

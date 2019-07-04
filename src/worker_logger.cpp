#include "worker_logger.h"
#include "worker_logger_manager.h"
#include "worker_logger_buffer.h"

worker_logger::worker_logger(worker_logger_manager* mgr, const std::filesystem::path& path, std::uint8_t idx) 
: idx_(0)
, path_(path)
, wlb_(new worker_logger_buffer(mgr))
, oss_(new std::ostream(wlb_.get())) {

}

worker_logger::worker_logger(worker_logger_manager* mgr, const std::filesystem::path& path, std::uint8_t idx, bool local)
: idx_(0)
, path_(path) {
    if(path.string() != "<clog>") {
        auto fb = new std::filebuf();
        fb->open(path, std::ios_base::app);
        oss_.reset(new std::ostream(fb));
        wlb_.reset(fb);
    }
}

std::ostream& worker_logger::stream() {
    return oss_ ? (*oss_) : std::clog;
}

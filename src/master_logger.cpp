#include "master_logger.h"
#include "coroutine.h"

void master_logger::reload() {
    file_.reset(new std::ofstream(path_, std::ios_base::out | std::ios_base::app));
    if (file_->fail()) // 文件打开失败时不会抛出异常，需要额外的状态检查
        std::cerr << "(ERROR) failed to create/open logger: file cannot be created or accessed\n";
    else return;
    file_.reset(&std::clog, boost::null_deleter());
}

void master_logger::close() {
    file_.reset(&std::clog, boost::null_deleter());
}
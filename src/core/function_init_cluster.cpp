#include "function_init_cluster.h"
#include "context.hpp"
#include "util.hpp"

#include <boost/process.hpp>
#include <thread>
#include <algorithm>
#include <sstream>

namespace flame { namespace core {

    function_init_cluster::function_init_cluster() {
        start();
    }

    function_init_cluster::~function_init_cluster() {
        // 等待工作进程退出完毕
        status_ |= STATUS_STOPPING | STATUS_STOPPED;
        for(auto& child : child_) {
            if(child && child->joinable()) child->join();
        }
    }

    void function_init_cluster::start() {
        // 注意 $->env.get("xxxxxx") 字符串包含 = 前缀（STRANGE）
        unsigned int count = std::atoi($->env["FLAME_MAX_WORKERS"].to_string().c_str());
        if(count == 0) {
            count = std::max(std::thread::hardware_concurrency()/2, 2u);
        }
        child_.resize(count);
        for(int i=0;i<count;++i) fork(i);
    }

    void function_init_cluster::halt() {
        status_ |= STATUS_STOPPING;
        // 强制终止
        for(auto& child : child_) {
            if(child) child->terminate();
        }
    }

    std::string function_init_cluster::cmdl() {
        std::stringstream ss;
        for(const char* arg : php::runtime::argv()) {
            ss << arg << " ";
        }
        return ss.str();
    }

    void function_init_cluster::fork(int index) {
        // 子进程环境变量标记
        boost::process::environment env {$->env};
        env["FLAME_CUR_WORKER"] = std::to_string(index+1);
        // 这里使用指针创建进程，停止时须判定指针有效性 @see on_exit
        child_[index].reset(new boost::process::child($->io_m, cmdl(), env,
            boost::process::std_out > stdout, boost::process::std_err > stderr,
            boost::process::on_exit = [this, index] (int exit_code, const std::error_code &error) {
                // 异常的停止流程（进程已经消失），忽略即可
                if(error.value() == static_cast<int>(std::errc::no_child_process)) return;
                this->on_exit(index, exit_code);
            }));
    }

    void function_init_cluster::on_exit(int index, int exit_code) {
        if(STATUS_STOPPING & status_ || exit_code == 0) {
            // 正常退出, 注意：停止时须判定指针有效性
            child_[index].reset();
        }
        else {
            // 适当等待后重启进程
            $->ev_after(std::chrono::milliseconds($random.i32(400,1000)), [this, index] () {
                if(STATUS_STOPPING & status_) return;
                fork(index);
            });
        }
    }
}}
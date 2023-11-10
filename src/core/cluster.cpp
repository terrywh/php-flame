#include "cluster.h"
#include "context.h"

namespace core {

    // 计算目标工作进程数量
    unsigned int cluster::evaluate_child_process_count() {
        unsigned int count = std::max(std::thread::hardware_concurrency()/2, 1u);
        if($context->env.count("FLAME_MAX_WORKERS") > 0) {
            unsigned int max = std::atoi( $context->env.at("FLAME_MAX_WORKERS").to_string().c_str() );
            // 需要将进程数量控制在合理范围内
            if(max >= 1 && max <= 256) count = max;
        }
        return count;
    }

    cluster::child_process::child_process(unsigned int count, const std::string& cmd)
    : cmd_(cmd) {
        child_.resize(count);
        for(int i=0;i<count;++i) on_fork(i); // 在主进程启动工作进程
    }

    cluster::child_process::~child_process() {
        $context->status.state |= context::STATE_STOP;
        // 若不等待子进程，子进程会脱离本进程组独立存在
        for(auto& c: child_) if(c && c->joinable()) c->join();
        // flame 框架功能仅在工作进程可用
        exit(0);
    }

    void cluster::child_process::on_fork(int index) {
        // 使用特殊的环境变量标记子进程
        boost::process::environment env {boost::this_process::environment()};
        env["FLAME_CUR_WORKER"] = std::to_string(index+1);
        // 注意：child_ 容器大小固定，但进程可能未创建
        child_[index].reset(new boost::process::child($context->io_m, cmd_, env,
            boost::process::std_out > stdout, boost::process::std_err > stderr,
            boost::process::on_exit = [this, index] (int exit_code, const std::error_code &error) {
                // 异常的停止流程（进程已经消失），忽略即可
                if(error.value() == static_cast<int>(std::errc::no_child_process)) return;
                this->on_exit(index, exit_code);
            }));
    }

    void cluster::child_process::on_exit(int index, int exit_code) {
        // 正常退出
        if($context->in_state(context::STATE_STOP) || exit_code == 0)
            child_[index].reset();
        else {
            // 异常退出：适当等待后重启进程
            std::uniform_int_distribution<int> dist {400, 100};
            $context->ev_after(std::chrono::milliseconds( dist($context->random) ), [this, index] () {
                // 等待时间内状态可能变化，需要重新检查
                if($context->in_state(context::STATE_STOP)) return;
                on_fork(index);
            });
        }
    }

    void cluster::child_process::stop() {
        $context->status.state |= context::STATE_STOP;
        for(auto &c : child_) c->terminate();
        // 由于 “正常停止” 在绝大多数场景需要用户逻辑介入，即：处理完现有逻辑、不在接受新请求等；
        // 故这里仅实现”强制终止“或”异常终止“；
    }

    cluster::worker_thread::worker_thread(unsigned int count)
    : guard_($context->io_w.get_executor()) {
        // 工作线程需要 guard 保护，否则会立即结束
        for(int i=0;i<count;++i)
            worker_.push_back(std::thread{[] () {
                $context->io_w.run();
            }});
    }

    cluster::worker_thread::~worker_thread() {
        guard_.reset();
        // 错误状态，立即终止所有可能执行中的异步流程
        if($context->in_state(context::STATE_ERROR)) $context->io_w.stop();
        for(auto& w: worker_) w.join();
    }

    
    cluster::handle_signal::handle_signal(cluster::child_process* cp, cluster::worker_thread* wt) {
        
    }
}

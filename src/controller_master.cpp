#include "controller_master.h"
#include "controller.h"

PHPAPI extern char *php_ini_opened_path;

namespace flame {

    static std::string start_command_line() {
        std::ostringstream ss;
        ss << php::constant("PHP_BINARY");
        ss << " -c " << php_ini_opened_path;
        php::array argv = php::server("argv");
        for (auto i = argv.begin(); i != argv.end(); ++i) ss << " " << i->second;
        return ss.str();
    }

    controller_master_worker::controller_master_worker(controller_master* m, int i)
    : m_(m), i_(i)
    , sout_(gcontroller->context_x), eout_(gcontroller->context_x) {
        // 准备命令行及环境变量（完全重新启动一个新的 PHP， 通过环境变量标识其为工作进程）
        std::string cmd = start_command_line();
        boost::process::environment env = gcontroller->env;
        env["FLAME_CUR_WORKER"] = std::to_string(i + 1);
        // 构造进程
        proc_ = boost::process::child(gcontroller->context_x, cmd, env,
            boost::process::std_out > sout_, boost::process::std_err > eout_,
            // 结束回调
            boost::process::on_exit = [this, i](int exit_code, const std::error_code &error) {
                if (error.value() == static_cast<int>(std::errc::no_child_process)) return;
                m_->notify_exit(i, this, exit_code);
            });
        redirect_output(sout_, sbuf_);
        redirect_output(eout_, ebuf_);
    }

    void controller_master_worker::redirect_output(boost::process::async_pipe& pipe, boost::asio::streambuf& buffer) {
        boost::asio::async_read_until(pipe, buffer, '\n', [this, &pipe, &buffer] (const boost::system::error_code &error, std::size_t nread) {
            if (error == boost::asio::error::operation_aborted || error == boost::asio::error::eof)
                ; // std::cerr << "(INFO) IGNORE sout\n";
            else if (error)
                std::cerr << "(ERROR) Failed to read from worker process: (" << error.value() << ") " << error.message() << "\n";
            else {
                auto y = buffer.data();
                *(m_->offile_) << std::string(boost::asio::buffers_begin(y), boost::asio::buffers_begin(y) + nread);
                buffer.consume(nread);
                m_->offile_->flush();

                redirect_output(pipe, buffer);
            }
        });
    }

    void controller_master_worker::close() {
        ::kill(proc_.id(), SIGTERM);
    }

    void controller_master_worker::close_now() {
        proc_.terminate();
        sout_.cancel();
        eout_.cancel();
        // 注意: 上面强制进程终止过程 proc_.terminate() 流程不会触发 on_exit 回调
        // 注意：不能在当前流程直接调用 notify_exit 否则会导致错误的状态使用: RESTART + SINGLE FORCEFUL CLOSE
        // m_->notify_exit(i_, this, 0);
        boost::asio::post(std::bind(&controller_master::notify_exit, m_, i_, this, 0));
    }

    controller_master::controller_master()
    : worker_spawn_(gcontroller->worker_size, nullptr)
    , worker_close_(gcontroller->worker_size, nullptr)
    , worker_spawn_count_(0)
    , worker_count_(0)
    , tm_inter_(gcontroller->context_x)
    , tm_force_(gcontroller->context_x) {

    }

    void controller_master::initialize(const php::array& options) {
        if (options.exists("logger")) ofpath_ = options.get("logger").to_string();
        else ofpath_ = "stdout";
        reload_output();
    }

    void controller_master::spawn_worker_next(int i) {
        if (i >= gcontroller->worker_size) return;
        if (i == 0) worker_spawn_count_ = 0;
        spawn_worker(i);

        // 防止启动过快，按间隔进行启动
        tm_inter_.cancel();
        tm_inter_.expires_after(std::chrono::milliseconds(125));
        tm_inter_.async_wait([this, i] (const boost::system::error_code& error) {
            if (error) return;
            spawn_worker_next(i + 1);
        });
    }

    void controller_master::spawn_worker(int i) {
        worker_spawn_[i] = new controller_master_worker(this, i);
        ++worker_count_;
        if(++worker_spawn_count_ == gcontroller->worker_size) { // 所有进程进行了启动或重启
            if((close_ & CLOSE_FLAG_MASK) != CLOSE_FLAG_MASTER) close_ = CLOSE_NOTHING; // 重启流程完毕, 重置状态
        }
    }

    void controller_master::close_worker(int i) {
        if (worker_close_[i] == nullptr) return;
        if ((close_ & CLOSE_WORKER_MASK) == CLOSE_WORKER_TERMINATE) worker_close_[i]->close_now();
        else worker_close_[i]->close();
    }

    void controller_master::close_worker_next(int i) {
        if (i == 0) worker_spawn_count_ = 0;
        if (i >= gcontroller->worker_size) {
            // 非重启模式, 所有进程都进行了关闭动作, 统一进行停止超时
            if((close_ & CLOSE_FLAG_MASK) != CLOSE_FLAG_RESTART
                && (close_ & CLOSE_WORKER_MASK) == CLOSE_WORKER_SIGNALING) {

                tm_force_.cancel();
                tm_force_.expires_after(std::chrono::milliseconds(gcontroller->worker_quit));
                tm_force_.async_wait([this] (const boost::system::error_code& error) {
                    if(error) return;
                    // 切换关闭模式
                    close_ =  (close_ & CLOSE_FLAG_MASK) | CLOSE_WORKER_TERMINATE;
                    close_worker_next();
                });
            }
            return;
        }

        close_worker(i);
        // 重启模式, 下一个进程的关闭将在本进程结束后开始
        // 重启模式, 按单个进程设置超时
        // if ((close_ & CLOSE_FLAG_MASK) == CLOSE_FLAG_RESTART
            // && (close_ & CLOSE_WORKER_MASK) == CLOSE_WORKER_SIGNALING) {
        if (close_ == (CLOSE_FLAG_RESTART | CLOSE_WORKER_SIGNALING)) {
            tm_force_.cancel();
            tm_force_.expires_after(std::chrono::milliseconds(gcontroller->worker_quit));
            tm_force_.async_wait([this, i] (const boost::system::error_code& error) {
                if(error) return;
                // 切换关闭模式, 立即结束进程
                close_ = (close_ & CLOSE_FLAG_MASK) | CLOSE_WORKER_TERMINATE;
                close_worker(i);
                // 恢复关闭模式, 本进程结束后继续下一个进程关闭
                close_ = (close_ & CLOSE_FLAG_MASK) | CLOSE_WORKER_SIGNALING;
            });
        }
        else { // 非重启模式, 继续下一个进程
            tm_inter_.cancel();
            tm_inter_.expires_after(std::chrono::milliseconds(125));
            tm_inter_.async_wait([this, i] (const boost::system::error_code& error) {
                if (error) return;
                close_worker_next(i + 1);
            });
        }
    }

    void controller_master::notify_exit(int i, controller_master_worker* w, int exit_code) {
        // 异常退出
        if (exit_code != 0 && (close_ & CLOSE_FLAG_MASK) != CLOSE_FLAG_MASTER) {
            int sec = std::rand() % 5 + 1;
            std::cerr << "[" << system_time() << "] (FATAL) worker unexpected exit: (" << exit_code << "), restart in " << sec << " ...\n";
            // 少许延迟进行重启
            auto tm = std::make_shared<boost::asio::steady_timer>(gcontroller->context_x, std::chrono::seconds(sec));
            tm->async_wait([this, tm, i](const boost::system::error_code &error) {
                if (error) std::cerr << "[" << system_time()
                    << "] (FATAL) failed to restart worker: ("
                    << error.value() << ") " << error.message() << std::endl;
                else if((close_ & CLOSE_FLAG_MASK) != CLOSE_FLAG_MASTER) spawn_worker(i); // 重启进程
            });
        }
        // 下面容器理论上仅会命中一个
        if (worker_spawn_[i] == w) worker_spawn_[i] = nullptr;
        if (worker_close_[i] == w) worker_close_[i] = nullptr;
        delete w;
        --worker_count_;
        // 正常退出
        if (exit_code == 0 || (close_ & CLOSE_FLAG_MASK) == CLOSE_FLAG_MASTER) {
            // 所有进程全部关闭, 同时计划关闭主进程
            if (worker_count_ == 0 && (close_ & CLOSE_FLAG_MASK) == CLOSE_FLAG_MASTER) close();
            // 重启模式, 关闭了一个进程
            // if ((close_ & CLOSE_FLAG_MASK) == CLOSE_FLAG_RESTART
            // && (close_ & CLOSE_WORKER_MASK) == CLOSE_WORKER_SIGNALING) {
            if (close_ == (CLOSE_FLAG_RESTART | CLOSE_WORKER_SIGNALING)) {
                tm_force_.cancel(); // 取消超时强制
                spawn_worker(i); // 重启子进程
                // 继续关闭下一个进程
                tm_inter_.cancel();
                tm_inter_.expires_after(std::chrono::milliseconds(125));
                tm_inter_.async_wait([this, i] (const boost::system::error_code& error) {
                    if (error) return;
                    close_worker_next(i + 1);
                });
            }
        }
    }

    void controller_master::close() {
        // 强制模式，子进程已经全部被结束了 -> 主进退出
        gcontroller->context_x.stop();
    }

    void controller_master::await_signal() {
        signal_->async_wait([this](const boost::system::error_code &error, int sig) {
            if (error) return;
             // 主进程与子进程 signal 处理之间有干扰
            await_signal();
            boost::asio::post(gcontroller->context_x, [this, sig]() {
                if (sig == SIGUSR1) {
                    if (close_ != CLOSE_NOTHING) return;
                    close_ = CLOSE_FLAG_RESTART | CLOSE_WORKER_SIGNALING;
                    worker_close_.swap(worker_spawn_); // 准备关闭的进程
                    close_worker_next(); // 关闭现有进程
                }
                else if (sig == SIGUSR2) reload_output();
                else {
                    if ((close_ & CLOSE_FLAG_MASK) == CLOSE_FLAG_MASTER) {
                        close_ = CLOSE_FLAG_MASTER | CLOSE_WORKER_TERMINATE;
                        close_worker_next();
                    }
                    else if (close_ == CLOSE_NOTHING) {
                        close_ = CLOSE_FLAG_MASTER | CLOSE_WORKER_SIGNALING;
                        worker_close_.swap(worker_spawn_); // 准备关闭的进程
                        close_worker_next();
                    }
                }
                // signal_.reset();
            });
        });
    }
    // !!! run 实际上是在 initialize 中执行 (防止实际启用了功能)
    void controller_master::run() {
        // 主进程的启动过程:
        // 1. 新增环境变量及命令行参数, 启动子进程;
        for(int i=0;i<gcontroller->worker_size;++i) spawn_worker(i);
        // 2. 监听信号进行日志重载或停止
        signal_.reset(new boost::asio::signal_set(gcontroller->context_y, SIGINT, SIGTERM));
        signal_->add(SIGUSR1); // 进程重载
        signal_->add(SIGUSR2); // 日志重载
        await_signal();
        // 3. 启动运行
        thread_ = std::thread([this] {
            gcontroller->context_y.run();
        });
        gcontroller->context_x.run();
        // 4. 等待工作线程结束
        if (signal_) signal_.reset();
        thread_.join();
    }

    void controller_master::reload_output() {
        if (ofpath_[0] == '/') {
            offile_.reset(new std::ofstream(ofpath_, std::ios_base::out | std::ios_base::app));
            // 文件打开失败时不会抛出异常，需要额外的状态检查
            if (!(*offile_))
                std::cerr << "(ERROR) failed to create/open logger target file: fallback to standard output\n";
            else return;
        }
        offile_.reset(&std::clog, boost::null_deleter());
    }
    // 住进成没有进行 time 明明空间相关的初始化，系统时间需要额外手段获取
    const char* controller_master::system_time() {
        static char buffer[24] = {0};
        std::time_t t = std::time(nullptr);
        struct tm *m = std::localtime(&t);
        sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
                1900 + m->tm_year,
                1 + m->tm_mon,
                m->tm_mday,
                m->tm_hour,
                m->tm_min,
                m->tm_sec);
        return buffer;
    }

}

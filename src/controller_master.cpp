#include "controller_master.h"
#include "controller.h"

namespace flame
{
    controller_master::controller_master()
        : worker_(gcontroller->worker_size)
        , sout_(gcontroller->worker_size)
        , sbuf_(gcontroller->worker_size)
        , eout_(gcontroller->worker_size)
        , ebuf_(gcontroller->worker_size)
    {

    }
    void controller_master::initialize(const php::array& options)
    {
        if(options.exists("logger"))
        {
            ofpath_ = options.get("logger").to_string();
        }
        else
        {
            ofpath_ = "stdout";
        }
        reload_output();
    }
    static std::string start_command_line()
    {
        std::ostringstream ss;
        ss << php::constant("PHP_BINARY");
        php::array argv = php::server("argv");
        for (auto i = argv.begin(); i != argv.end(); ++i)
        {
            ss << " " << i->second;
        }
        return ss.str();
    }
    void controller_master::spawn_worker(int i)
    {
        sout_[i].reset( new boost::process::async_pipe(gcontroller->context_x) );
        eout_[i].reset( new boost::process::async_pipe(gcontroller->context_x) );

        ++count_;
        std::string cmd = start_command_line();
        boost::process::environment env = gcontroller->env;
        env["FLAME_CUR_WORKER"] = std::to_string(i + 1);

        worker_[i].reset( new boost::process::child(cmd, env/*, group_*/, gcontroller->context_x,
            boost::process::std_out > *sout_[i],
            boost::process::std_err > *eout_[i],
            // boost::process::std_out > boost::process::null,
            boost::process::on_exit = [this, i](int exit_code, const std::error_code &error) {
                if (error.value() == static_cast<int>(std::errc::no_child_process))
                {
                    return;
                }
                if (exit_code != 0)
                {
                    // 日志记录
                    int sec = std::rand() % 3 + 3;
                    std::cerr << "(FATAL) worker unexpected exit (" << exit_code << "), restart in " << sec << " ...\n";

                    auto tm = std::make_shared<boost::asio::steady_timer>(gcontroller->context_x, std::chrono::seconds(sec));
                    tm->async_wait([this, tm, i](const boost::system::error_code &error) {
                        if (error)
                        {
                            std::cerr << "(FATAL) failed to restart worker: (" << error.value() << ") " << error.message() << std::endl;
                        }
                        else
                        {
                            spawn_worker(i);
                        }
                    });
                }
                worker_[i].reset();
                sout_[i].reset();
                eout_[i].reset();
            }) );
        redirect_sout(i);
        redirect_eout(i);
    }
    void controller_master::redirect_sout(int i)
    {
        boost::asio::async_read_until(*sout_[i], sbuf_[i], '\n', [this, i](const boost::system::error_code &error, std::size_t nread) {
            if (error == boost::asio::error::operation_aborted || error == boost::asio::error::eof)
            {
                // std::cerr << "(INFO) IGNORE sout\n";
            }
            else if(error)
            {
                std::cerr << "(ERROR) failed to read from worker process: (" << error.value() << ") " << error.message() << std::endl;
            }
            else
            {
                auto y = sbuf_[i].data();
                *offile_ << std::string(boost::asio::buffers_begin(y), boost::asio::buffers_begin(y) + nread);
                sbuf_[i].consume(nread);
                offile_->flush();

                redirect_sout(i);
            }
        });
    }
    void controller_master::redirect_eout(int i)
    {
        boost::asio::async_read_until(*eout_[i], ebuf_[i], '\n', [this, i](const boost::system::error_code &error, std::size_t nread) {
            if (error == boost::asio::error::operation_aborted || error == boost::asio::error::eof)
            {
                // std::cerr << "(INFO) IGNORE eout\n";
            }
            else if (error)
            {
                std::cerr << "(ERROR) failed to read from worker process: (" << error.value() << ") " << error.message() << std::endl;
            }
            else
            {
                auto y = ebuf_[i].data();
                *offile_ << std::string(boost::asio::buffers_begin(y), boost::asio::buffers_begin(y) + nread);
                ebuf_[i].consume(nread);
                offile_->flush();
                
                redirect_eout(i);
            }
        });
    }
    void controller_master::await_signal()
    {
        signal_->async_wait([this](const boost::system::error_code &error, int sig) {
            if (error) return;
            await_signal();
            boost::asio::post(gcontroller->context_x, [this, sig]() {
                if (sig == SIGUSR2)
                {
                    reload_output();
                }
                else
                {
                    close_worker();
                    // 似乎主进程与子进程 signal 处理之间有干扰
                    // 需要后置清理
                    // signal_.reset();
                }
            });
        });
    }
    // !!! run 实际上是在 initialize 中执行 (防止实际启用了功能)
    void controller_master::run()
    {
        // 主进程的启动过程:
        // 1. 新增环境变量及命令行参数, 启动子进程;
        for(int i=0;i<gcontroller->worker_size;++i)
        {
            spawn_worker(i);
        }
        // 2. 监听信号进行日志重载或停止
        signal_.reset(new boost::asio::signal_set(gcontroller->context_y, SIGINT, SIGTERM, SIGUSR2));
        await_signal();
        // 3. 启动运行
        thread_ = std::thread([this] {
            gcontroller->context_y.run();
        });
        gcontroller->context_x.run();
        // 4. 等待工作线程结束
        if (signal_) signal_.reset();
        if (timer_) timer_.reset();
        thread_.join();
        // 5. 强制进程结束
        for(auto i=worker_.begin();i!=worker_.end();++i) {
            if(*i) {
                (*i)->terminate();
            }
        }
        // if(!group_.wait_for(std::chrono::seconds(1))) group_.terminate();
    }
    void controller_master::reload_output()
    {
        if(ofpath_[0] == '/') {
            offile_.reset(new std::ofstream(ofpath_, std::ios_base::out | std::ios_base::app));
            // 文件打开失败时不会抛出异常，需要额外的状态检查
			if(!(*offile_)) {
                std::cerr << "(ERROR) failed to create/open logger target file, fallback to standard output\n";
            }else{
                return;
            }
		}
        offile_.reset(&std::clog, boost::null_deleter());
    }
    void controller_master::close_worker()
    {
        for (auto i = worker_.begin(); i != worker_.end(); ++i)
        {
            if (*i)
            {
                ::kill((*i)->id(), SIGTERM);
            }
        }
        timer_.reset(new boost::asio::steady_timer(gcontroller->context_y));
        timer_->expires_after(std::chrono::seconds(10));
        timer_->async_wait([] (const boost::system::error_code& error) {
            if(error) return;
            gcontroller->context_x.stop();
        });
    }
}

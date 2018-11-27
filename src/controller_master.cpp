#include "controller_master.h"
#include "controller.h"

namespace flame
{
    controller_master::controller_master()
        : signal_(gcontroller->context_y, SIGINT, SIGTERM, SIGUSR2)
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
        // 主进程实际上直接运行
        run();
        // 直接退出
        exit(0);
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
        ++count_;
        std::string cmd = start_command_line();

        boost::process::environment env = gcontroller->env;
        env["FLAME_PROCESS_WORKER"] = std::to_string(i + 1);
        worker_[i] = boost::process::child(cmd, env, group_, gcontroller->context_x,
            boost::process::std_out > pipe_[i],
            boost::process::std_err > pipe_[i],
            boost::process::on_exit = [this, i](int exit_code, const std::error_code &error) {
                if (error.value() == static_cast<int>(std::errc::no_child_process))
                    return;
                if (exit_code != 0)
                {
                    // 日志记录
                    int sec = std::rand() % 3 + 2;
                    boost::asio::async_write(pipe_[i],
                        boost::asio::buffer((boost::format("(FATAL) worker unexpected exit, restart in %2%s ...") % sec).str()),
                        [this, i, sec](const boost::system::error_code &error, std::size_t nsent) {
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
                        });
                }
                else
                {
                    pipe_[i].close();
                }
            });
        
    }
    void controller_master::redirect_output(int i)
    {
        boost::asio::async_read_until(pipe_[i], buff_[i], '\n', [this, i] (const boost::system::error_code& error, std::size_t nread)
        {
            if(error == boost::asio::error::operation_aborted) 
            {

            }
            else if(error)
            {
                std::cerr << "(ERROR) failed to read from worker process\n";
            }
            else
            {
                for (auto x = boost::asio::buffer_sequence_begin(buff_[i].data()); x != boost::asio::buffer_sequence_end(buff_[i].data()); ++x)
                {
                    offile_->write((char*)x->data(), x->size());
                }
                buff_[i].consume(nread);
                redirect_output(i);
            }
        });
    }
    // !!! run 实际上是在 initialize 中执行 (防止实际启用了功能)
    void controller_master::run()
    {
        // 主进程的启动过程:
        // 1. 新增环境变量及命令行参数, 启动子进程;
        for(int i=0;i<gcontroller->worker_size;++i)
        {
            pipe_.emplace_back(gcontroller->context_x);
            spawn_worker(i);
            redirect_output(i);
        }
        // 2. 监听信号进行日志重载或停止
        signal_.async_wait([this] (const boost::system::error_code &error, int sig) {
            if(sig == SIGUSR2)
            {
                reload_output();
            }
            else
            {
                signal_.clear();
            }
        });
        // 3. 启动运行
        thread_ = std::thread([this] {
            gcontroller->context_y.run();
        });
        gcontroller->context_x.run();
        // 4. 等待工作线程结束
        signal_.clear();
        thread_.join();
        // 5. 等待工作进程结束
        std::error_code error;
        if(!group_.wait_for(std::chrono::seconds(10), error))
        {
            group_.terminate();
        }
    }
    void controller_master::reload_output()
    {
        if(ofpath_[0] == '/') {
            offile_.reset(new std::ofstream(ofpath_, std::ios_base::out | std::ios_base::app));
            // 文件打开失败时不会抛出异常，需要额外的状态检查
			if(!(*offile_)) {
                std::clog << "(ERROR) failed to create/open logger target file, fallback to standard output\n";
            }else{
                return;
            }
		}
        
        offile_.reset(&std::clog, boost::null_deleter());
    }
}
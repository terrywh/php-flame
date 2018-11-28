#include "controller_master.h"
#include "controller.h"

namespace flame
{
    controller_master::controller_master()
        : signal_(new boost::asio::signal_set(gcontroller->context_y, SIGINT, SIGTERM, SIGUSR2))
        , worker_(gcontroller->worker_size)
        , sbuf_(gcontroller->worker_size)
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
        ++count_;
        std::string cmd = start_command_line();
        boost::process::environment env = gcontroller->env;
        env["FLAME_CUR_WORKER"] = std::to_string(i + 1);

        worker_[i].reset( new boost::process::child(cmd, env, group_, gcontroller->context_x,
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
                    int sec = std::rand() % 3 + 2;
                    std::cerr << "(FATAL) worker unexpected exit, restart in " << sec << " ...\n";

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
                else
                {
                    sout_[i]->close();
                    eout_[i]->close();
                }
            }) );
        
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
                for (auto x = boost::asio::buffer_sequence_begin(y); x != boost::asio::buffer_sequence_end(y); ++x)
                {
                    offile_->write((char*)x->data(), x->size());
                }
                offile_->flush();
                sbuf_[i].consume(nread);

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
                for (auto x = boost::asio::buffer_sequence_begin(y); x != boost::asio::buffer_sequence_end(y); ++x)
                {
                    offile_->write((char *)x->data(), x->size());
                }
                offile_->flush();
                ebuf_[i].consume(nread);

                redirect_eout(i);
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
            sout_.push_back(std::make_unique<boost::process::async_pipe>(gcontroller->context_x));
            eout_.push_back(std::make_unique<boost::process::async_pipe>(gcontroller->context_x));
            spawn_worker(i);
            redirect_sout(i);
            redirect_eout(i);
        }
        // 2. 监听信号进行日志重载或停止
        signal_->async_wait([this] (const boost::system::error_code &error, int sig) {
            if(sig == SIGUSR2)
            {
                reload_output();
            }
            else
            {
                // 信号的默认处理: 转给所有子进程
                for(auto i=worker_.begin(); i!=worker_.end(); ++i)
                {
                    ::kill( (*i)->id(), sig );
                }
            }
        });
        // 3. 启动运行
        thread_ = std::thread([this] {
            gcontroller->context_y.run();
        });
        gcontroller->context_x.run();
        // 4. 等待工作线程结束
        signal_.reset();
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
                std::cout << "(ERROR) failed to create/open logger target file, fallback to standard output\n";
            }else{
                return;
            }
		}
        
        offile_.reset(&std::cout, boost::null_deleter());
    }
}
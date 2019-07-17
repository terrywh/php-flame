#include "master_process.h"
#include "master_process_manager.h"
#include "master_logger.h"
#include "util.h"

extern "C" {
    PHPAPI extern char *php_ini_opened_path;
}

static std::string php_cmd() {
    std::ostringstream ss;
    ss << php::constant("PHP_BINARY");
    ss << " -c " << php_ini_opened_path;
    php::array argv = php::server("argv");
    for (auto i = argv.begin(); i != argv.end(); ++i) ss << " " << i->second;
    return ss.str();
}

master_process::master_process(boost::asio::io_context& io, master_process_manager* mgr, std::uint8_t idx)
: mgr_(mgr), idx_(idx)
, sout_(io), eout_(io) {
    // 准备命令行及环境变量（完全重新启动一个新的 PHP， 通过环境变量标识其为工作进程）
    boost::process::environment env = boost::this_process::environment();
    env["FLAME_CUR_WORKER"] = std::to_string(idx + 1);
    // 构造进程
    proc_ = boost::process::child(io, php_cmd(), env,
        boost::process::std_out > sout_, boost::process::std_err > eout_,
        // boost::process::std_out > boost::process::null, boost::process::std_err > boost::process::null,
        // boost::process::std_out > stdout, boost::process::std_err > stdout,
        // 结束回调
        boost::process::on_exit = [this, &io] (int exit_code, const std::error_code &error) {
            if (error.value() == static_cast<int>(std::errc::no_child_process)) return;
            boost::asio::post(io, [exit_code, this] () {
                mgr_->on_child_close(this, exit_code == 0);
            });
        });
    redirect_output(sout_, sbuf_);
    redirect_output(eout_, ebuf_);

    boost::asio::post(io, [this] () {
        mgr_->on_child_start(this);
    });
}

void master_process::redirect_output(boost::process::async_pipe& pipe, std::string& data) {
    boost::asio::async_read_until(pipe, boost::asio::dynamic_buffer(data), '\n', [this, &pipe, &data] (const boost::system::error_code &error, std::size_t nread) {
        if (error == boost::asio::error::operation_aborted || error == boost::asio::error::eof)
            ; // std::cout << "redirect_output stopped" << std::endl; // 忽略
        else if (error) {
            mgr_->output() << "[" << util::system_time() << "] (ERROR) Failed to read from worker process: (" << error.value() << ") " << error.message() << "\n";
        }
        else {
            mgr_->output() << /*"-----" << */std::string_view(data.data(), nread);
            data.erase(0, nread);
            redirect_output(pipe, data);
        }
    });
}

void master_process::close(bool force) {
    if(force) proc_.terminate();
    else ::kill(proc_.id(), SIGTERM);
}

void master_process::signal(int sig) {
    ::kill(proc_.id(), sig);
}
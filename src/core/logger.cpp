#include "logger.hpp"

#include "context.hpp"
#include <algorithm>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


namespace flame { namespace core {

    class php_logger: public php::class_basic<php_logger> {
    public:
        php::value __construct(php::parameters& params) {
            php::value file = php::to_string(params[0]);
            php::value opts;
            if(params.size() > 1) opts = params[1];
            else opts = php::array::create();

            sinker_ = & $logger.open(
                file.as<php::string>().c_str(), // 文件路径
                !!opts.as<php::array>().get("fifo") // 作为管道
            );
            return nullptr;
        }

        php::value __destruct(php::parameters& params) {
            // $logger.close(*sinker_);
            return nullptr;
        }

        php::value write(php::parameters& params) {
            std::stringstream ss;
            for(int i=0;i<params.size();++i) {
                ss << params[i];
                // 所有输出项间使用 " " 分隔，每次 write 操作附加换行
                ss.put(i == params.size() - 1 ? '\n' : ' ');
            }
            sinker_->write(ss.str());
            return nullptr;
        }
    private:
        logger::sinker* sinker_;
    };

    static logger::sinker* $sinker;

    php::value trace(php::parameters& params) {
        
        return nullptr;
    }

    php::value debug(php::parameters& params) {

        return nullptr;
    }

    php::value info(php::parameters& params) {

        return nullptr;
    }

    php::value warn(php::parameters& params) {

        return nullptr;
    }
    php::value error(php::parameters& params) {

        return nullptr;
    }
    php::value fatal(php::parameters& params) {

        return nullptr;
    }

    void logger::declare(php::module_entry& module) {
        module
            - php::function<trace>("flame\\trace", {
                {"data", /* byref */ false, /* nullable */true, /* variadic */true},
            })
            - php::function<debug>("flame\\debug", {
                {"data", /* byref */ false, /* nullable */true, /* variadic */true},
            })
            - php::function<info>("flame\\info", {
                {"data", /* byref */ false, /* nullable */true, /* variadic */true},
            })
            - php::function<warn>("flame\\warn", {
                {"data", /* byref */ false, /* nullable */true, /* variadic */true},
            })
            - php::function<error>("flame\\error", {
                {"data", /* byref */ false, /* nullable */true, /* variadic */true},
            })
            - php::function<fatal>("flame\\fatal", {
                {"data", /* byref */ false, /* nullable */true, /* variadic */true},
            });
        // 类声明
        module.declare<php_logger>("flame\\logger")
            - php::method<&php_logger::__construct>("__construct", {
                {"file", php::TYPE_STRING},
                {"opts", php::TYPE_ARRAY, /* byref */ false, /* nullable */ true},
            })
            - php::method<&php_logger::__destruct>("__destruct")
            - php::method<&php_logger::write>("write", {
                {"data", false, false, true},
            });

        $->on_flame_init.connect([] () {
            std::cout << "flame_init\n";
        });
    }
    // 全局日志对象
    logger $logger;

    logger::logger() {

    }

    logger::sinker& logger::open(int fd) {
        auto hint = sinker_.try_emplace({"<internal>"}, reinterpret_cast<char*>(fd), false);
        return hint.first->second;
    }

    logger::sinker& logger::open(std::filesystem::path file, bool fifo) {
        auto hint = sinker_.try_emplace(file, file.c_str(), fifo);
        return hint.first->second;
    }

    // 关闭指定的 sinker 对象
    void logger::close(logger::sinker& s) {
        auto fi = std::find_if(sinker_.begin(), sinker_.end(), [&s] (typename decltype(sinker_)::value_type entry) {
            return entry.second == s;
        });
        if(fi != sinker_.end()) sinker_.erase(fi);
    }

    logger::sinker::sinker(const char* file, bool fifo) {
        if(fifo) {
            // @see https://www.man7.org/linux/man-pages/man3/mkfifo.3.html
            // 注意：同一进程对同一通道进行读取、写入操作可能导致死锁
            if(0 != ::mkfifo(file, file_mode) && errno != EEXIST) {
                throw php::error("failed to create fifo pipe");
            }
        }
        switch(reinterpret_cast<uintptr_t>(file)) {
        case 1:
        case 2:
            file_ = reinterpret_cast<uintptr_t>(file);
        default:
            // @see https://www.man7.org/linux/man-pages/man2/open.2.html
            // 注意：在 NFS 文件系统上追加流程可能工作不正常
            file_ = ::open(file, O_APPEND | O_CREAT | O_WRONLY, file_mode);
        }
        if(file_ <= 0) {
            throw php::error("failed to open logger sink file");
        }
    }

    void logger::sinker::write(std::string_view data) const {
        // @see https://www.man7.org/linux/man-pages/man2/write.2.html
        // 系统操作可保证原子性（移动到文件末尾+写入数据）
        ::write(file_, data.data(), data.size());
    }

    bool logger::sinker::operator ==(const sinker& s) const {
        return s.file_ == file_;
    }

    logger::sinker::~sinker() {
        // 关闭文件
        if(file_ > 2) ::close(file_);
    }
}}

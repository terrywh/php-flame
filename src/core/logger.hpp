#ifndef PHP_FLAME_CORE_LOG_H
#define PHP_FLAME_CORE_LOG_H

#include <phpext.h>

#include <filesystem>
#include <map>
#include <string_view>

#include <fcntl.h>

namespace flame { namespace core {

    class logger {
    public:
        // 目标数据写入
        class sinker {
        private:
            int file_;

            static constexpr int file_mode = S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH;
        public:
            // 建立对指定文件的追加写入目标
            sinker(const char* file, bool fifo = false);
            //
            ~sinker();
            // 追加
            void write(std::string_view log) const;

            bool operator ==(const sinker& s) const;
        };
    private:
        // 
        std::map<std::filesystem::path, sinker> sinker_;
    public:
        // 
        logger();
        // 标准输出、错误输出
        sinker& open(int fd);
        // 打开一个指定路径的 sinker 写入
        sinker& open(std::filesystem::path file, bool fifo = false);
        // 关闭指定的 sinker 对象
        void close(sinker& s);

        static void declare(php::module_entry& module);
    };

    extern logger $logger;
}}

#endif // PHP_FLAME_CORE_LOG_H

#ifndef PHP_FLAME_CORE_CLUSTER_H
#define PHP_FLAME_CORE_CLUSTER_H

#include <boost/process.hpp>
#include <memory>

namespace flame { namespace core {
    class function_init_cluster {
    public:
        function_init_cluster();
        ~function_init_cluster();
        void start();
        void halt();
    private:
        std::string cmdl();
        void fork(int index);
        void on_exit(int index, int exit_code);
        std::vector<std::unique_ptr<boost::process::child>> child_;

        enum {
            STATUS_STOPPING = 1,
            STATUS_STOPPED,
        };
        int status_ = 0;
    };
}}


#endif // PHP_FLAME_CORE_CLUSTER_H

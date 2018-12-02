#pragma once
#include "vendor.h"

namespace flame
{
    class controller_master {
	public:
		controller_master();
        void initialize(const php::array& options);
		void run();
    private:
		std::vector<std::unique_ptr<boost::process::child>>    worker_;
        std::vector<std::unique_ptr<boost::process::async_pipe>> sout_;
        std::vector<std::unique_ptr<boost::process::async_pipe>> eout_;
        std::vector<boost::asio::streambuf> sbuf_;
        std::vector<boost::asio::streambuf> ebuf_;
        boost::process::group               group_;
		std::unique_ptr<boost::asio::signal_set> signal_;
		std::thread                        thread_;
        std::size_t                        count_;

        std::string                        ofpath_;
        std::shared_ptr<std::ostream>      offile_;

        void spawn_worker(int i);
        void reload_output();
        void redirect_sout(int i);
        void redirect_eout(int i);
    };
}
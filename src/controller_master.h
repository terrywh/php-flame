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
		std::vector<boost::process::child> worker_;
        std::vector<boost::process::async_pipe> pipe_;
        std::vector<boost::asio::streambuf> buff_;
		boost::process::group               group_;
		boost::asio::signal_set            signal_;
		std::thread                        thread_;
        std::size_t                        count_;

        std::string                        ofpath_;
        std::shared_ptr<std::ostream>      offile_;

        void spawn_worker(int i);
        void reload_output();
        void redirect_output(int i);
    };
}
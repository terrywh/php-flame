
// class fastcgi_session {
// public:
// 	fastcgi_session(mill_unixsock sock);
// 	void run();
// private:
// 	mill_unixsock sock_;
// 	FCGI_Header   head_;
// 	char          body_[64 * 1024];

// 	int flag_;

// 	void on_begin_request();
// 	void on_params();
// 	void on_stdin();
// };
// #include "vendor.h"
// #include "fastcgi_session.h"

// fastcgi_session::fastcgi_session(mill_unixsock sock)
// :sock_(sock) {

// }

// void fastcgi_session::run() {
// 	size_t nbytes;
// READ_NEXT:
// 	nbytes = mill_unixrecv(sock_, &head_, sizeof(head_), -1);
// 	if(errno != 0) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		goto DESTROY;
// 	}
// 	if(head_.version != FCGI_VERSION_1) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		goto DESTROY;
// 	}
// 	head_.content_length = ntohs(head_.content_length);
// 	nbytes = mill_unixrecv(sock_, body_, head_.content_length + head_.padding_length, -1);
// 	if(errno != 0) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		return;
// 	}
// 	switch(head_.type) {
// 	case FCGI_BEGIN_REQUEST:
// 		this->on_begin_request();
// 	break;
// 	case FCGI_PARAMS:
// 		this->on_params();
// 	break;
// 	case FCGI_STDIN:
// 		this->on_stdin();
// 	break;
// 	default:
// 		std::printf("unknown type: %d\n", head_.type);
// 	}
// 	if(errno != 0) {
// 		std::printf("error: %d %s\n", errno, strerror(errno));
// 		goto DESTROY;
// 	}
// 	goto READ_NEXT;
// DESTROY:
// 	mill_unixclose(sock_);
// 	delete this;
// }
// void fastcgi_session::on_begin_request() {
// 	FCGI_BeginRequestBody* body = reinterpret_cast<FCGI_BeginRequestBody*>(body_);
// 	flag_ = body->flags;
// }
// void fastcgi_session::on_params() {
	
// }
// void fastcgi_session::on_stdin() {
// 	if(head_.content_length == 0) { // 请求结束了

// 		head_.type = FCGI_STDOUT;
// 		head_.content_length = htons(40);
// 		head_.padding_length = 0;
// 		mill_unixsend(sock_, &head_, FCGI_HEADER_LEN, -1);
// 		mill_unixsend(sock_, "Content-type: text/plain\r\n\r\nHello World!", 40, -1);
		
// 		head_.type = FCGI_STDOUT;
// 		head_.content_length = 0;
// 		head_.padding_length = 0;
// 		mill_unixsend(sock_, &head_, FCGI_HEADER_LEN, -1);

// 		head_.type = FCGI_END_REQUEST;
// 		head_.content_length = htons( sizeof(FCGI_EndRequestBody) );
// 		head_.padding_length = 0;
// 		mill_unixsend(sock_, &head_, FCGI_HEADER_LEN, -1);

// 		FCGI_EndRequestBody* body = reinterpret_cast<FCGI_EndRequestBody*>(body_);
// 		body->app_status      = htons(0);
// 		body->protocol_status = FCGI_REQUEST_COMPLETE;
// 		body->reserved[0] = '\0';
// 		body->reserved[0] = '\0';
// 		body->reserved[0] = '\0';	
// 		mill_unixsend(sock_, body, sizeof(FCGI_EndRequestBody), -1);

// 		mill_unixflush(sock_, -1);
// 		if(!(flag_ & FCGI_KEEP_CONN)) {
// 			mill_unixclose(sock_);
// 		}
// 	}
// }
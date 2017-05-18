#pragma once

namespace net { namespace http {
	class server_request;
	class client_request;
	class server_response;
	class client_response;
	class header: public php::class_base {
	public:
		static void init(php::extension_entry& extension);
		void init(evkeyvalq* headers);
		// Iterator
		php::value current(php::parameters& params);
		php::value key(php::parameters& params);
		php::value next(php::parameters& params);
		php::value rewind(php::parameters& params);
		php::value valid(php::parameters& params);
		// ArrayAccess
		php::value offsetExists(php::parameters& params);
		php::value offsetGet(php::parameters& params);
		php::value offsetSet(php::parameters& params);
		php::value offsetUnset(php::parameters& params);
		php::value append(php::parameters& params);
		php::value remove(php::parameters& params);
	private:
		evkeyvalq* queue_;
		evkeyval*  item_;
		friend class server_request;
		friend class client_request;
		friend class server_response;
		friend class client_response;
	};
}}

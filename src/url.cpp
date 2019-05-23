#include "vendor.h"
#include "url.h"

namespace flame {
    typedef ::parser::separator_parser<std::string, std::string> parser_t;

    url::url(const php::string &str, bool parse_query)
    : raw_(str) {
        auto u = php::parse_url(str);
        schema.assign(u->scheme);
        if(u->user) user.assign(u->user);
        if(u->pass) pass.assign(u->pass);
        if(u->host) host.assign(u->host);
        port = u->port;
        if(u->path) path.assign(u->path);
        if(u->query && parse_query) {
            parser_t p('\0', '\0', '=', '\0', '\0', '&', [this](parser_t::entry_type et) {
                php::url_decode_inplace(et.second.data(), et.second.size());
                query[et.first] = et.second;
                std::cout << et.first << ": " << et.second << "\n";
            });
            p.parse(u->query, strlen(u->query));
            p.end();
        }
    }

    std::string url::str(bool with_query, bool update) {
        if (update) {
            std::ostringstream ss;
            ss << schema << "://" << user << ":" << pass << "@" << host << ":" << port << path;
            if (with_query) {
                ss << "?";
                for (auto i=query.begin();i!=query.end();++i) {
                    ss << i->first << "=" << php::url_encode(i->second.c_str(), i->second.size()) << "&";
                }
            }
            raw_ = ss.str();
        }
        return raw_;
    }

} // namespace flame
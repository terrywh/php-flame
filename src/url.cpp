#include "vendor.h"
#include "url.h"

namespace flame {
    typedef ::parser::separator_parser<std::string, std::string> parser_t;

    url::url(const php::string &str, bool parse_query)
    : raw_(str) {
        const char* s = str.c_str();

        struct http_parser_url x;
        http_parser_parse_url(s, str.size(), 0, &x);
        if (x.field_set & (1 << UF_SCHEMA)) {
            auto y = x.field_data[UF_SCHEMA];
            schema.assign(s + y.off, y.len);
        }
        if (x.field_set & (1 << UF_USERINFO)) {
            auto y = x.field_data[UF_USERINFO];
            int i = 0;
            for(;i<y.len;++i) {
                if (s[y.off + i] == ':') break;
                else user.push_back(s[y.off + i]);
            }
            ++i;
            for (;i<y.len;++i) {
                pass.push_back(s[y.off + i]);
            }
        }
        if (x.field_set & (1 << UF_HOST)) {
            auto y = x.field_data[UF_HOST];
            host.assign(s + y.off, y.len);
        }
        port = x.port;
        if (x.field_set & (1 << UF_PATH)) {
            auto y = x.field_data[UF_PATH];
            path.assign(s + y.off, y.len);
        }
        if (x.field_set & (1 << UF_QUERY)) {
            auto y = x.field_data[UF_QUERY];
            if (parse_query) {
                parser_t p('\0', '\0', '=', '\0', '\0', '&', [this](parser_t::entry_type et) {
                    std::string key = et.first;
                    std::string val = et.second;
                    val.resize(php::url_decode_inplace(val.data(), val.size()));
                    query[key] = val;
                });
                p.parse(s + y.off, y.len);
                p.end();
            }
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
#include "_connection_base.h"

namespace flame::mysql
{
    void _connection_base::escape(std::shared_ptr<MYSQL> conn, php::buffer &b, const php::value &v, char quote)
    {
        switch (Z_TYPE_P(static_cast<zval *>(v)))
        {
        case IS_NULL:
            b.append("NULL", 4);
            break;
        case IS_TRUE:
            b.append("TRUE", 4);
            break;
        case IS_FALSE:
            b.append("FALSE", 5);
            break;
        // case IS_LONG:
        // case IS_DOUBLE: {
        // 	php::string str = v;
        // 	str.to_string();
        // 	b.append(str);
        // 	break;
        // }
        case IS_STRING:
        {
            php::string str = v;
            // 支持字段名 aaa.bbb 进行 ESCAPE 变为 `aaa`.`bbb`
            if (quote == '`')
            {
                const char *s = str.c_str(), *c, *e = str.c_str() + str.size();
                for (c = s; c < e; ++c)
                {
                    if (*c == '.')
                    {
                        escape(conn, b, php::string(s, c - s), quote);
                        b.push_back('.');
                        escape(conn, b, php::string(c + 1, e - c - 1), quote);
                        goto ESCAPE_FINISHED;
                    }
                }
            }
            char *to = b.prepare(str.size() * 2 + 2);
            std::size_t n = 0;
            to[n++] = quote;
            // 摘自 mysql_real_escape_string_quote() @ libmysql.c:1228 相关流程
            if (conn->server_status & SERVER_STATUS_NO_BACKSLASH_ESCAPES)
            {
                n += escape_quotes_for_mysql(conn->charset, to + 1, str.size() * 2 + 1, str.c_str(), str.size(), quote);
            }
            else
            {
                n += escape_string_for_mysql(conn->charset, to + 1, str.size() * 2 + 1, str.c_str(), str.size());
            }
            to[n++] = quote;
            b.commit(n);
            break;
        }
        case IS_ARRAY:
        {
            php::array arr = v;
            int index = 0;
            b.push_back('(');
            for (auto i = arr.begin(); i != arr.end(); ++i)
            {
                if (++index > 1)
                    b.push_back(',');
                escape(conn, b, i->second, quote);
            }
            b.push_back(')');
            break;
        }
        default:
        {
            php::string str = v;
            str.to_string();
            escape(conn, b, str, quote);
        }
        }
ESCAPE_FINISHED:;
    }
} // namespace flame::mysql

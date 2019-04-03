#include "../controller.h"
#include "../coroutine.h"
#include "mysql.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "client.h"
#include "result.h"
#include "tx.h"

namespace flame::mysql {

    void declare(php::extension_entry &ext)
    {
        gcontroller
            ->on_init([] (const php::array& options)
            {
                mysql_library_init(0, nullptr, nullptr);
            })
            ->on_stop([] ()
            {
                mysql_library_end();
            });
        ext
            .function<connect>("flame\\mysql\\connect",
            {
                {"url", php::TYPE::STRING},
            });
        client::declare(ext);
        result::declare(ext);
        tx::declare(ext);
    }

    php::value connect(php::parameters& params)
    {
        url u(params[0]);
        if(u.port < 10) u.port = 3306;
        if(!u.query.count("charset")) {
            u.query["charset"] = "utf8mb4";
        }
        if(!u.query.count("ssl")) {
            u.query["ssl"] = "disabled";
        }

        php::object obj(php::class_entry<client>::entry());
        client *ptr = static_cast<client *>(php::native(obj));
        ptr->cp_.reset(new _connection_pool(u));
        ptr->cp_->sweep(); // 启动自动清理扫描
        // TODO 优化: 确认第一个连接建立 ?
        return std::move(obj);
	}
    // 相等
    static void where_eq(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        if (cond.typeof(php::TYPE::NULLABLE))
        {
            buf.append(" IS NULL", 8);
        }
        else
        {
            php::string str = cond;
            str.to_string();
            buf.push_back('=');
            _connection_base::escape(cc, buf, str);
        }
    }
    // 不等 {!=}
    static void where_ne(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        if (cond.typeof(php::TYPE::NULLABLE))
        {
            buf.append(" IS NOT NULL", 12);
        }
        else
        {
            php::string str = cond;
            str.to_string();
            buf.append("!=", 2);
            _connection_base::escape(cc, buf, str);
        }
    }
    // 大于 {>}
    static void where_gt(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        if (cond.typeof(php::TYPE::NULLABLE))
        {
            buf.append(">0", 2);
        }
        else
        {
            php::string str = cond;
            str.to_string();
            buf.push_back('>');
            _connection_base::escape(cc, buf, str);
        }
    }
    // 小于 {<}
    static void where_lt(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        if (cond.typeof(php::TYPE::NULLABLE))
        {
            buf.append("<0", 2);
        }
        else
        {
            php::string str = cond;
            str.to_string();
            buf.push_back('<');
            _connection_base::escape(cc, buf, str);
        }
    }
    // 大于等于 {>=}
    static void where_gte(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        if (cond.typeof(php::TYPE::NULLABLE))
        {
            buf.append(">=0", 3);
        }
        else
        {
            php::string str = cond;
            str.to_string();
            buf.append(">=", 2);
            _connection_base::escape(cc, buf, str);
        }
    }
    // 小于等于 {<=}
    static void where_lte(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        if (cond.typeof(php::TYPE::NULLABLE))
        {
            buf.append("<=0", 3);
        }
        else
        {
            php::string str = cond;
            str.to_string();
            buf.append("<=", 2);
            _connection_base::escape(cc, buf, str);
        }
    }
    // 某个 IN 无对应符号
    static void where_in(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::array &cond)
    {
        assert(cond.typeof(php::TYPE::ARRAY) && "目标格式错误");

        buf.append(" IN (", 5);
        for (auto i = cond.begin(); i != cond.end(); ++i)
        {
            if (static_cast<int>(i->first) > 0)
            {
                buf.push_back(',');
            }
            php::string v = i->second;
            v.to_string();
            _connection_base::escape(cc, buf, v);
        }
        buf.push_back(')');
    }
    // WITH IN RANGE
    static void where_wir(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::array &cond)
    {
        assert(cond.typeof(php::TYPE::ARRAY) && "目标格式错误");

        buf.append(" BETWEEN ", 9);
        std::int64_t min = std::numeric_limits<std::int64_t>::max(), max = std::numeric_limits<std::int64_t>::min();
        for (auto i = cond.begin(); i != cond.end(); ++i)
        {
            std::int64_t x = i->second.to_integer();
            if (x > max)
                max = x;
            if (x < min)
                min = x;
        }
        buf.append(std::to_string(min));
        buf.append(" AND ", 5);
        buf.append(std::to_string(max));
    }
    // OUT OF RANGE
    static void where_oor(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::array &cond)
    {
        buf.append(" NOT", 4);
        where_wir(cc, buf, cond);
    }
    // LINK
    static void where_lk(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        buf.append(" LIKE ", 6);
        _connection_base::escape(cc, buf, cond);
    }
    // NOT LIKE
    static void where_nlk(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &cond)
    {
        buf.append(" NOT LIKE ", 10);
        _connection_base::escape(cc, buf, cond);
    }
    static void where_ex(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::array &cond, const php::string &field, const php::string &separator)
    {
        if (cond.size() > 1)
            buf.push_back('(');

        int j = -1;
        for (auto i = cond.begin(); i != cond.end(); ++i)
        {
            if (++j > 0)
                buf.append(separator);

            if (i->first.typeof(php::TYPE::INTEGER))
            {
                if (i->second.typeof(php::TYPE::ARRAY))
                {
                    where_ex(cc, buf, i->second, i->first, " AND ");
                }
                else
                {
                    php::string str = i->second;
                    str.to_string();
                    buf.append(str);
                }
            }
            else
            {
                php::string key = i->first;
                if (key.c_str()[0] == '{')
                { // OPERATOR (php::TYPE::STRING)
                    if ((key.size() == 5 && strncasecmp(key.c_str(), "{NOT}", 5) == 0) || (key.size() == 3 && strncasecmp(key.c_str(), "{!}", 3) == 0))
                    {
                        assert(i->second.typeof(php::TYPE::ARRAY));
                        buf.append(" NOT ", 5);
                        where_ex(cc, buf, i->second, php::string(nullptr), " AND ");
                    }
                    else if (key.size() == 4 && (strncasecmp(key.c_str(), "{OR}", 4) == 0 || strncasecmp(key.c_str(), "{||}", 4) == 0))
                    {
                        assert(i->second.typeof(php::TYPE::ARRAY));
                        where_ex(cc, buf, i->second, php::string(nullptr), " OR ");
                    }
                    else if ((key.size() == 5 && strncasecmp(key.c_str(), "{AND}", 5) == 0) || (key.size() == 4 && strncasecmp(key.c_str(), "{&&}", 4) == 0))
                    {
                        assert(i->second.typeof(php::TYPE::ARRAY));
                        where_ex(cc, buf, i->second, php::string(nullptr), " AND ");
                    }
                    else if (key.size() == 4 && strncasecmp(key.c_str(), "{!=}", 4) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_ne(cc, buf, i->second);
                    }
                    else if (key.size() == 3 && strncasecmp(key.c_str(), "{>}", 3) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_gt(cc, buf, i->second);
                    }
                    else if (key.size() == 3 && strncasecmp(key.c_str(), "{<}", 2) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_lt(cc, buf, i->second);
                    }
                    else if (key.size() == 4 && strncasecmp(key.c_str(), "{>=}", 4) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_gte(cc, buf, i->second);
                    }
                    else if (key.size() == 4 && strncasecmp(key.c_str(), "{<=}", 4) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_lte(cc, buf, i->second);
                    }
                    else if (key.size() == 4 && strncasecmp(key.c_str(), "{<>}", 4) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_wir(cc, buf, i->second);
                    }
                    else if (key.size() == 4 && strncasecmp(key.c_str(), "{><}", 4) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_oor(cc, buf, i->second);
                    }
                    else if (key.size() == 3 && strncasecmp(key.c_str(), "{~}", 3) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_lk(cc, buf, i->second);
                    }
                    else if (key.size() == 4 && strncasecmp(key.c_str(), "{!~}", 4) == 0)
                    {
                        _connection_base::escape(cc, buf, field, '`');
                        where_nlk(cc, buf, i->second);
                    }
                }
                else
                { // php::TYPE::STRING
                    if (i->second.typeof(php::TYPE::ARRAY))
                    {
                        php::array cond = i->second;
                        if (cond.exists(0))
                        {
                            _connection_base::escape(cc, buf, key, '`');
                            buf.push_back(' ');
                            where_in(cc, buf, i->second);
                        }
                        else
                        {
                            where_ex(cc, buf, cond, key, " AND ");
                        }
                    }
                    else
                    {
                        _connection_base::escape(cc, buf, key, '`');
                        where_eq(cc, buf, i->second);
                    }
                }
            }
        }
        if (cond.size() > 1)
            buf.push_back(')');
    }
    void build_where(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &data)
    {
        buf.append(" WHERE ", 7);
        if (data.typeof(php::TYPE::STRING) || data.typeof(php::TYPE::INTEGER))
        {
            buf.push_back(' ');
            buf.append(data);
        }
        else if (data.typeof(php::TYPE::ARRAY))
        {
            where_ex(cc, buf, data, php::string(0), " AND ");
        }
        else
        {
            buf.push_back('1');
        }
    }
    void build_order(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &data)
    {
        if (data.typeof(php::TYPE::STRING))
        {
            buf.append(" ORDER BY ", 10);
            buf.append(data);
        }
        else if (data.typeof(php::TYPE::ARRAY))
        {
            buf.append(" ORDER BY", 9);
            php::array order = data;
            int j = -1;
            for (auto i = order.begin(); i != order.end(); ++i)
            {
                if (++j > 0)
                    buf.push_back(',');
                if (i->first.typeof(php::TYPE::INTEGER))
                {
                    if (i->second.typeof(php::TYPE::STRING))
                    {
                        buf.push_back(' ');
                        buf.append(i->second);
                    }
                }
                else
                {
                    buf.push_back(' ');
                    _connection_base::escape(cc, buf, i->first, '`');
                    if (i->second.typeof(php::TYPE::STRING))
                    {
                        buf.push_back(' ');
                        buf.append(i->second);
                    }
                    else if (i->second.typeof(php::TYPE::YES) || (i->second.typeof(php::TYPE::INTEGER) && static_cast<int>(i->second) >= 0))
                    {
                        buf.append(" ASC", 4);
                    }
                    else
                    {
                        buf.append(" DESC", 5);
                    }
                }
            }
        }
        else
        {
            // throw php::exception(zend_ce_type_error, "failed to build 'ORDER BY' clause: unsupported type");
        }
    }
    void build_limit(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::value &data)
    {
        if (data.typeof(php::TYPE::STRING) || data.typeof(php::TYPE::INTEGER))
        {
            buf.append(" LIMIT ", 7);
            buf.append(data);
        }
        else if (data.typeof(php::TYPE::ARRAY))
        {
            buf.append(" LIMIT", 6);
            php::array limit = data;
            int j = -1;
            for (auto i = limit.begin(); i != limit.end(); ++i)
            {
                if (++j > 0)
                    buf.push_back(',');
                buf.push_back(' ');
                buf.append(i->second);
                if (j > 0)
                    break;
            }
        }
        else
        {
            throw php::exception(zend_ce_type_error, "failed to build 'LIMIT' clause: unsupported type specified for `limit`");
        }
    }
    static void insert_row(std::shared_ptr<MYSQL> cc, php::buffer &buf, const php::array &row)
    {
        int j = -1;
        for (auto i = row.begin(); i != row.end(); ++i)
        {
            if (++j > 0)
                buf.append(", ", 2);
            _connection_base::escape(cc, buf, i->second);
        }
    }
    void build_insert(std::shared_ptr<MYSQL> cc, php::buffer &buf, php::parameters &params)
    {
        buf.append("INSERT INTO ", 12);
        // 表名
        _connection_base::escape(cc, buf, params[0], '`');
        // 字段
        buf.push_back('(');
        php::array rows = params[1], row;
        if (rows.exists(0))
        {
            row = rows.get(0);
            assert(!row.exists(0) && "二级行无字段");
        }
        else
        {
            row = rows;
            rows = nullptr;
        }
        assert(row.typeof(php::TYPE::ARRAY) && "行非数组");
        int j = -1;
        for (auto i = row.begin(); i != row.end(); ++i)
        {
            if (++j > 0)
                buf.append(", ", 2);
            _connection_base::escape(cc, buf, i->first, '`');
        }
        // 数据
        buf.append(") VALUES", 8);
        if (rows.typeof(php::TYPE::ARRAY))
        {
            buf.push_back('(');
            int j = -1;
            for (auto i = rows.begin(); i != rows.end(); ++i)
            {
                if (++j > 0)
                    buf.append("), (", 4);
                insert_row(cc, buf, i->second);
            }
            buf.push_back(')');
        }
        else
        {
            buf.push_back('(');
            insert_row(cc, buf, row);
            buf.push_back(')');
        }
    }
    void build_delete(std::shared_ptr<MYSQL> cc, php::buffer &buf, php::parameters &params)
    {
        buf.append("DELETE FROM ", 12);
        // 表名
        _connection_base::escape(cc, buf, params[0], '`');
        // 条件
        build_where(cc, buf, params[1]);
        // 排序
        if (params.size() > 2)
            build_order(cc, buf, params[2]);
        // 限制
        if (params.size() > 3)
            build_limit(cc, buf, params[3]);
    }
    void build_update(std::shared_ptr<MYSQL> cc, php::buffer &buf, php::parameters &params)
    {
        buf.append("UPDATE ", 7);
        // 表名
        _connection_base::escape(cc, buf, params[0], '`');
        // 数据
        buf.append(" SET", 4);
        php::array data = params[2];
        if (data.typeof(php::TYPE::STRING))
        {
            buf.push_back(' ');
            buf.append(data);
        }
        else if (data.typeof(php::TYPE::ARRAY))
        {
            int j = -1;
            for (auto i = data.begin(); i != data.end(); ++i)
            {
                if (++j > 0)
                    buf.push_back(',');
                buf.push_back(' ');
                _connection_base::escape(cc, buf, i->first, '`');
                buf.push_back('=');
                _connection_base::escape(cc, buf, i->second);
            }
        }
        else throw php::exception(zend_ce_type_error, "failed to build 'UPDATE' clause: unsupported type specified for `update`");
        // 条件
        build_where(cc, buf, params[1]);
        // 排序
        if (params.size() > 3)
            build_order(cc, buf, params[3]);
        // 限制
        if (params.size() > 4)
            build_limit(cc, buf, params[4]);
    }
    void build_select(std::shared_ptr<MYSQL> cc, php::buffer &buf, php::parameters &params)
    {
        buf.append("SELECT ", 7);
        // 字段
        php::value fields = params[1];
        if (fields.typeof(php::TYPE::STRING)) buf.append(fields);
        else if (fields.typeof(php::TYPE::ARRAY)) {
            php::array a = fields;
            int j = -1;
            for (auto i = a.begin(); i != a.end(); ++i)
            {
                if (++j > 0)
                    buf.append(", ", 2);
                if (i->first.typeof(php::TYPE::INTEGER))
                {
                    _connection_base::escape(cc, buf, i->second, '`');
                }
                else
                {
                    buf.append(i->first);
                    buf.push_back('(');
                    _connection_base::escape(cc, buf, i->second, '`');
                    buf.push_back(')');
                }
            }
        }
        else if(fields.typeof(php::TYPE::NULLABLE)) buf.push_back('*');
        else throw php::exception(zend_ce_type_error, "failed to build 'SELECT' clause: unsupported type specified for `fields`");
        // 表名
        buf.append(" FROM ", 6);
        _connection_base::escape(cc, buf, params[0], '`');
        // 条件
        if (params.size() > 2)
            build_where(cc, buf, params[2]);
        // 排序
        if (params.size() > 3)
            build_order(cc, buf, params[3]);
        // 限制
        if (params.size() > 4)
            build_limit(cc, buf, params[4]);
    }
    void build_one(std::shared_ptr<MYSQL> cc, php::buffer &buf, php::parameters &params)
    {
        buf.append("SELECT * FROM ", 14);
        // 表名
        _connection_base::escape(cc, buf, params[0], '`');
        // 条件
        if (params.size() > 1)
            build_where(cc, buf, params[1]);
        // 排序
        if (params.size() > 2)
            build_order(cc, buf, params[2]);
        buf.append(" LIMIT 1", 8);
    }
    void build_get(std::shared_ptr<MYSQL> cc, php::buffer &buf, php::parameters &params)
    {
        buf.append("SELECT ", 7);
        // 字段
        if (params[1].typeof(php::TYPE::STRING)) _connection_base::escape(cc, buf, params[1], '`');
        else throw php::exception(zend_ce_type_error, "failed to build 'SELECT' clause: unsupported type specified for `field`");
        // 表名
        buf.append(" FROM ", 6);
        _connection_base::escape(cc, buf, params[0], '`');
        // 条件
        if (params.size() > 2)
            build_where(cc, buf, params[2]);
        // 排序
        if (params.size() > 3)
            build_order(cc, buf, params[3]);
        buf.append(" LIMIT 1", 8);
    }
}

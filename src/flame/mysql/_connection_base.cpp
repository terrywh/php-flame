#include "../controller.h"
#include "_connection_base.h"
#include "_connection_lock.h"
#include "result.h"

namespace flame::mysql {
    
    void _connection_base::escape(std::shared_ptr<MYSQL> conn, php::buffer &b, const php::value &v, char quote) {
        switch (Z_TYPE_P(static_cast<zval *>(v))) {
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
        //  php::string str = v;
        //  str.to_string();
        //  b.append(str);
        //  break;
        // }
        case IS_STRING: {
            // 支持字段名 aaa.bbb 进行 ESCAPE 变为 `aaa`.`bbb`
            if (quote == '`') {
                std::string str = v; // 这里进行拷贝（原字符串可能发生更改）
                char *s = str.data(), *c, *e = str.data() + str.size();
                char *t = b.prepare(str.size() * 2 + 2);
                std::size_t n = 0;
ESCAPE_REMAINING:
                for (c = s; c < e; ++c) {
                    if (*c == '`') *c = '\\';
                    if (*c == '.') {
                        t[n++] = quote;
                        n += mysql_real_escape_string(conn.get(), t + n, s, c - s);
                        t[n++] = quote;
                        t[n++] = '.';
                        s = c + 1;
                        goto ESCAPE_REMAINING;
                    }
                }
                t[n++] = quote;
                n += mysql_real_escape_string(conn.get(), t + n, s, c - s);
                t[n++] = quote;
                b.commit(n);
            } else {
                php::string str = v;
                char *t = b.prepare(str.size() * 2 + 2);
                std::size_t n = 0;
                t[n++] = quote;
                n += mysql_real_escape_string(conn.get(), t + n, str.c_str(), str.size());
                t[n++] = quote;
                b.commit(n);
            }
            break;
        }
        case IS_ARRAY:
            // 实际数据项按 JSON 处理
            escape(conn, b, php::json_encode(v), quote);
            break;
        case IS_OBJECT: {
            php::object obj = v;
            php::string str;
            if (obj.instanceof(php_date_get_date_ce()))  // DateTime 类型的 SQL 拼接
                str = obj.call("format", {php::string("Y-m-d H:i:s")});
            else str = obj.to_string();
            escape(conn, b, str, quote);
            break;
        }
        default: {
            php::string str = v;
            str.to_string();
            escape(conn, b, str, quote);
        }
        }
    }

    php::string _connection_base::format(std::shared_ptr<MYSQL> conn, php::parameters& params) {
        php::buffer buf;
        php::string sql = params[0];
        int o = 0;
        char* data = sql.data();
        for(int i=0;i<sql.size();++i) {
            if(data[i] == '?') {
                ++o;
                if(params.size() > o) 
                    _connection_base::escape(conn, buf, params[o], '\'');
                else throw php::exception(zend_ce_type_error, "Failed to format query: more argument(s) required");
            }
            else
                buf.push_back(data[i]);
        }
        if(params.size() > o + 1) 
            zend_error(E_WARNING, "Failed to format query: extra format argument(s) detected");
        return std::move(buf);
    }

    php::object _connection_base::query(std::shared_ptr<MYSQL> conn, std::string sql, coroutine_handler& ch) {
        MYSQL_RES* rst = nullptr;
        int err = 0;
        boost::asio::post(gcontroller->context_y, [&err, &conn, &ch, &rst, &sql] () {
            // 在工作线程执行查询
            err = mysql_real_query(conn.get(), sql.c_str(), sql.size());
            if (err == 0) {
                // 防止锁表, 均使用 store 方式
                rst = mysql_store_result(conn.get());
                if (!rst && mysql_field_count(conn.get()) > 0) {
                    err = -1;
                }
            }
            ch.resume();
        });
        ch.suspend();
        last_query_ = sql;
        if (err != 0) {
            int err = mysql_errno(conn.get());
            throw php::exception(zend_ce_exception
                , (boost::format("Failed to query MySQL server: %s") % mysql_error(conn.get())).str()
                , err);
        }
        if (rst) { // 存在结果集
            php::object obj(php::class_entry<result>::entry());
            result* ptr = static_cast<result*>(php::native(obj));
            ptr->cl_.reset(new _connection_lock(conn));
            ptr->rs_.reset(rst, mysql_free_result);
            ptr->f_ = mysql_fetch_fields(rst);
            ptr->n_ = mysql_num_fields(rst);
            obj.set("stored_rows", static_cast<std::int64_t>(mysql_num_rows(rst)));
            return std::move(obj);
        }
        else { // 更新型操作
            php::array data(2);
            data.set(php::string("affected_rows", 13), static_cast<std::int64_t>(mysql_affected_rows(conn.get())));
            data.set(php::string("insert_id", 9), static_cast<std::int64_t>(mysql_insert_id(conn.get())));
            return std::move(data);
        }
    }

    const std::string& _connection_base::last_query() {
        return last_query_;
    }

    php::array _connection_base::fetch(std::shared_ptr<MYSQL> conn, std::shared_ptr<MYSQL_RES> rst
        , MYSQL_FIELD *f, unsigned int n, coroutine_handler &ch) {

        MYSQL_ROW row;
        unsigned long* len;
        boost::asio::post(gcontroller->context_y, [&rst, &ch, &row, &len] () {
            row = mysql_fetch_row(rst.get());
            if (row) len = mysql_fetch_lengths(rst.get());
            ch.resume();
        });
        ch.suspend();
        if (!row) {
            int err = mysql_errno(conn.get());
            if (err != 0)
                throw php::exception(zend_ce_error
                    , (boost::format("Failed to fetch MySQL row: %s") % mysql_error(conn.get())).str()
                    , err);
            else return nullptr;
        }
        php::array php_row {std::size_t(n)};
        for(int i=0;i<n;++i) {
            php::string field(f[i].name, f[i].name_length);
            php::value  value;
            if (row[i] == nullptr) {
                value = nullptr;
                php_row.set(field, value);
                continue;
            }
            switch(f[i].type) {
                case MYSQL_TYPE_DOUBLE:
                case MYSQL_TYPE_FLOAT:
                    value = std::strtod(row[i], nullptr);
                    break;
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_INT24:
                    value = std::strtol(row[i], nullptr, 10);
                    break;
                case MYSQL_TYPE_LONGLONG:
                    value = static_cast<std::int64_t>(std::strtoll(row[i], nullptr, 10));
                    break;
                case MYSQL_TYPE_DATETIME: {
                    php::object obj( php::CLASS{php_date_get_date_ce()} );
                    obj.call("__construct", {php::string(row[i], len[i])});
                    value = std::move(obj);
                    break;
                }
                case MYSQL_TYPE_JSON:
                    value = php::json_decode(row[i], len[i]);
                    break;
                default:
                    value = php::string(row[i], len[i]);
            }
            php_row.set(field, value);
        }
        return php_row;
    }
} // namespace flame::mysql

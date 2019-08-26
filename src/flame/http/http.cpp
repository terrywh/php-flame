#include "../coroutine.h"
#include "http.h"
#include "client.h"
#include "value_body.h"
#include "client_request.h"
#include "client_response.h"
#include "client_body.h"
#include "server.h"
#include "server_request.h"
#include "server_response.h"
#include "_handler.h"

namespace flame::http {
    
    client* client_;
    std::int64_t body_max_size = 1024 * 1024 * 1024;

    void declare(php::extension_entry& ext) {
        client_ = nullptr;
        ext.on_module_startup([] (php::extension_entry& ext) -> bool {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            return true;
        });
        // 在框架初始化后创建全局HTTP客户端
        gcontroller->on_init([] (const php::array& opts) {
            body_max_size = std::max(php::ini("post_max_size").calc(), body_max_size);
            client_ = new client();
            php::parameters p(0, nullptr);
            client_->__construct(p);
        })->on_stop([] {
            delete client_;
            client_ = nullptr;
        });
        ext
            .function<get>("flame\\http\\get")
            .function<post>("flame\\http\\post")
            .function<put>("flame\\http\\put")
            .function<delete_>("flame\\http\\delete")
            .function<exec>("flame\\http\\exec");

        client_request::declare(ext);
        client_response::declare(ext);
        client_body::declare(ext);
        client::declare(ext);
        server::declare(ext);
        server_request::declare(ext);
        server_response::declare(ext);
    }
    static void init_guard() {
        if (!client_)
            throw php::exception(zend_ce_error_exception
                , "Failed to execute HTTP request: exception or missing 'flame\\init()' ?"
                , -1);
    }
    php::value get(php::parameters& params) {
        init_guard();
        return client_->get(params);
    }
    php::value post(php::parameters& params) {
        init_guard();
        return client_->post(params);
    }
    php::value put(php::parameters& params) {
        init_guard();
        return client_->put(params);
    }
    php::value delete_(php::parameters& params) {
        init_guard();
        return client_->delete_(params);
    }
    php::value exec(php::parameters& params) {
        init_guard();
        return client_->exec(params);
    }
    php::string ctype_encode(std::string_view ctype, const php::value& v) {
        if (v.type_of(php::TYPE::STRING)) return v;

        php::value r = v;
        if (ctype.compare(0, 33, "application/x-www-form-urlencoded") == 0) {
            if (r.type_of(php::TYPE::ARRAY)) {
                r = php::callable("http_build_query")({v});
            }
            else {
                r.to_string();
            }
        }
        else if (ctype.compare(0, 16, "application/json") == 0) {
            r = php::json_encode(r);
        }
        else {
            r.to_string();
        }
        return r;
    }
    php::value ctype_decode(std::string_view ctype, const php::string& v, php::array* files) {
        if (ctype.compare(0, 16, "application/json") == 0) {
            return php::json_decode(v);
        }
        else if (ctype.compare(0, 33, "application/x-www-form-urlencoded") == 0) {
            php::array data(4);
            php::callable("parse_str")({v, data.make_ref()});
            return data;
        }
        else if (ctype.compare(0, 19, "multipart/form-data") == 0) {
            std::string boundary;
            std::string field;
            php::array meta(4);
            parser::separator_parser<std::string, std::string> p1('\0','\0','=','"','"',';', [&boundary, &field, &meta] (std::pair<std::string, std::string> entry) {
                if (entry.first == "boundary") {
                    boundary = std::move(entry.second);
                }
                else if (entry.first == "content-disposition") {
                    // 此项头信息没有用途
                }
                else if (entry.first == "name") {
                    // 这里可能有不严格的地方存在，即每个 Header 项后可能都存在对应的补充字段，且这些字段名称可能重复
                    // 表单字段名
                    field = std::move(entry.second);
                }
                else {
                    meta.set(entry.first, entry.second);
                }
            });
            std::size_t begin = ctype.find_first_of(';', 19) + 1;
            p1.parse(ctype.data() + begin, ctype.size() - begin);
            p1.parse(";", 1); // 保证一行 K/V 结束，复用 PARSER

            php::array data(8);
            parser::multipart_parser<std::string, php::buffer> p2(boundary, [&p1, &field, &meta, &data, &files] (std::pair<std::string, php::buffer> entry) {
                if (entry.first.size() == 0) { // Part Data
                    if (meta.exists("filename") > 0) {
                        if (files) { // 接收对应文件
                            // 上传文件 meta 
                            meta["size"] = entry.second.size();
                            meta["data"] = std::move(entry.second);
                            files->set(field, meta);
                        }
                    }
                    else {
                        data.set(field, std::move(entry.second));
                    }
                    meta = php::array(4);
                }
                else { // Part Header
                    php::lowercase_inplace(entry.first.data(), entry.first.size());
                    std::size_t end = std::string_view(entry.second.data(), entry.second.size()).find_first_of(';');
                    if (end == std::string::npos) {
                        meta[entry.first] = std::move(entry.second);
                    }
                    else if (entry.first == "content-disposition") {
                        // 这里可能有不严格的地方存在，即每个 Header 项后可能都存在对应的补充字段，且这些字段名称可能重复
                        std::size_t begin = end + 1;
                        if (entry.second.size() > begin + 2) {
                            p1.parse(entry.second.data() + begin, entry.second.size() - begin);
                            p1.parse(";", 1); // 保证一行 K/V 结束，复用 PARSER
                        }
                    }
                    else {
                        meta[entry.first] = php::string(entry.second.data(), end);
                    }
                }
            });
            p2.parse(v.data(), v.size());

            return data;
        }
        else return v;
    }
}

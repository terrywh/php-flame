#include "../coroutine.h"
#include "message.h"
#include "smtp.h"
#include "../time/time.h"

// MIME encode WORDS =?charset?encoding?encoded?=
// 使用 CURL 进行 SMTP 协议请求
namespace flame::smtp {

    void message::declare(php::extension_entry& ext) {
        php::class_entry<message> class_message(
            "flame\\smtp\\message");
        class_message
            .property({"timeout", 3000})
            .method<&message::__construct>("__construct", {})
            .method<&message::from>("from", {
                {"from", php::TYPE::ARRAY},
            })
            .method<&message::to>("to", {
                {"to", php::TYPE::ARRAY},
            })
            .method<&message::cc>("cc", {
                {"cc", php::TYPE::ARRAY},
            })
            .method<&message::subject>("subject", {
                {"subject", php::TYPE::STRING},
            })
            .method<&message::append>("append", {
                {"data", php::TYPE::STRING},
                {"opts", php::TYPE::ARRAY},
            })
            .method<&message::to_string>("__toString");
        ext.add(std::move(class_message));
    }

    php::value message::__construct(php::parameters& params) {
        
        return nullptr;
    }

    static const char random_chars[] = "ABCDEFGHIJKLMKOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678";
    std::string message::random(int size) {
        std::string r(size, '\0');
        for(int i=0;i<size;++i) {
            r[i] = random_chars[ rand() % sizeof(random_chars) ];
        }
        return r;
    }

    message::message()
    :  status_(0)
    , c_rcpt_(nullptr)
    , c_easy_(curl_easy_init()) {
        boundary_ = random(24);
        curl_easy_setopt(c_easy_, CURLOPT_READFUNCTION, read_cb);
        curl_easy_setopt(c_easy_, CURLOPT_READDATA, this);
        curl_easy_setopt(c_easy_, CURLOPT_UPLOAD, 1L);
    }
    message::~message() {
        if (c_rcpt_) curl_slist_free_all(c_rcpt_);
        if (c_easy_) curl_easy_cleanup(c_easy_); // 未执行的请求需要清理
    }
    // 来源发送者
    php::value message::from(php::parameters& params) {
        if((status_ & 0x011) > 0) {
            throw php::exception(zend_ce_error_exception, "Failed to set from: from/body already set");
        }
        php::array from = params[0];
        status_ |= 0x001;
        c_data_.append("From: ", 6);
        for(auto i=from.begin();i!=from.end();++i) {
            if(i->first.type_of(php::TYPE::STRING)) {
                php::string name = i->first.to_string(), mail = i->second.to_string();
                c_data_.append("\"=?UTF-8?B?", 11); 
                c_data_.append(php::base64_encode((const unsigned char*)name.data(), name.size()));
                c_data_.append("?=\" <", 5);
                c_data_.append(mail);
                c_data_.push_back('>');
            }
            else {
                php::string mail = i->second.to_string();
                c_data_.push_back('<');
                c_data_.append(mail);
                c_data_.push_back('>');
            }
            break; // 来源地址仅允许设置唯一一个
        }
        c_data_.append("\r\n", 2);
        return this;
    }
    // 目标接收者 1
    php::value message::to(php::parameters& params) {
        if((status_ & 0x012) > 0) {
            throw php::exception(zend_ce_error_exception, "Failed to set to: to/body already set");
        }
        php::array tos = params[0];
        status_ |= 0x002;
        c_data_.append("To: ", 4);
        int r = 0;
        for(auto i=tos.begin(); i!=tos.end(); ++i) {
            if(++r > 1) c_data_.push_back(',');
            if(i->first.type_of(php::TYPE::STRING)) {
                php::string name = i->first.to_string(), mail = i->second.to_string();
                c_data_.append("\"=?UTF-8?B?", 11); 
                c_data_.append(php::base64_encode((const unsigned char*)name.data(), name.size()));
                c_data_.append("?=\" <", 5);
                c_data_.append(mail);
                c_data_.push_back('>');
                c_rcpt_ = curl_slist_append(c_rcpt_, mail.data());
            }
            else {
                php::string mail = i->second.to_string();
                c_data_.push_back('<');
                c_data_.append(mail);
                c_data_.push_back('>');
                c_rcpt_ = curl_slist_append(c_rcpt_, mail.data());
            }
        }
        c_data_.append("\r\n", 2);
        return this;
    }
     // 目标接收者 2
    php::value message::cc(php::parameters& params) {
        if(status_ & 0x014) {
            throw php::exception(zend_ce_error_exception, "Failed to set subject: cc/body already set");
        }
        php::array ccs = params[0];
        status_ |= 0x004;
        c_data_.append("Cc: ", 4);
        int r = 0;
        for(auto i=ccs.begin(); i!=ccs.end(); ++i) {
            if(i->first.type_of(php::TYPE::STRING)) {
                php::string name = i->first.to_string(), mail = i->second.to_string();
                c_data_.append("\"=?UTF-8?B?", 11); 
                c_data_.append(php::base64_encode((const unsigned char*)name.data(), name.size()));
                c_data_.append("?=\" <", 5);
                c_data_.append(mail);
                c_data_.push_back('>');
                c_rcpt_ = curl_slist_append(c_rcpt_, mail.data());
            }
            else {
                php::string mail = i->second.to_string();
                c_data_.push_back('<');
                c_data_.append(mail);
                c_data_.push_back('>');
                c_rcpt_ = curl_slist_append(c_rcpt_, mail.data());
            }
        }
        c_data_.append("\r\n", 2);
        return this;
    }
    // 主题
    php::value message::subject(php::parameters& params) {
        if((status_ & 0x018) > 0) {
            throw php::exception(zend_ce_error_exception, "Failed to set subject: subject/body already set");
        }
        status_ |= 0x008;
        c_data_.append("Subject: =?UTF-8?B?", 19);
        php::string subject = params[0];
        c_data_.append(php::base64_encode((const unsigned char*)subject.data(), subject.size()));
        c_data_.append("?=\r\n", 4);
        return this;
    }
    // 添加内容
    php::value message::append(php::parameters& params) {
        if((status_ & 0x00b) != 0x00b) { // 必须先设置 HEADER 后设置 BODY
            throw php::exception(zend_ce_error_exception, "Failed to append body: headers not properly set");
        }
        if(!(status_ & 0x010)) { // 首个 BODY 补充必要的头信息
            c_data_.append("Date: ", 6);
            php::object date = php::datetime(std::chrono::duration_cast<std::chrono::milliseconds>(time::now().time_since_epoch()).count());
            c_data_.append(date.call("format", {"l, d M Y H:i:s \\G\\M\\T"}));
            c_data_.append("\r\n", 2);
            c_data_.append("Content-Type: multipart/alternative; boundary=\"", 47);
            c_data_.append(boundary_);
            c_data_.append("\"\r\n\r\n--", 7);
        }
        status_ |= 0x010;
        c_data_.append(boundary_);
        c_data_.append("\r\n", 2);

        php::array opts = params[1];
        for(auto i=opts.begin();i!=opts.end();++i) {
            c_data_.append(i->first);
            c_data_.append(": ", 2);
            c_data_.append(i->second);
            c_data_.append("\r\n", 2);
        }
        c_data_.append("Content-Transfer-Encoding: base64\r\n\r\n", 37);
        php::string data = params[0];
        c_data_.append(php::base64_encode((const unsigned char*)data.data(), data.size()));
        c_data_.append("\r\n--", 4);
        return this;
    }

    void message::build_ex() {
        if((status_ & 0x100) > 0) return;
        status_ |= 0x100;
        c_data_.append(boundary_); // 结束标记
        c_data_.append("--", 2);
        c_mail_ = php::string(std::move(c_data_));

        long timeout = static_cast<long>(get("timeout"));
        curl_easy_setopt(c_easy_, CURLOPT_MAIL_RCPT, c_rcpt_);
        curl_easy_setopt(c_easy_, CURLOPT_TIMEOUT_MS, timeout);
    }

    std::size_t message::read_cb(char* buffer, std::size_t size, std::size_t n, void *data) {
        
        message* self = (message*)data;
        std::size_t copy = std::min(size * n, self->c_mail_.size() - self->c_size_);
        if(copy > 0) {
            std::memcpy(buffer, self->c_mail_.data() + self->c_size_, copy);
            self->c_size_ += copy;
        }
        return copy;
    }

    php::value message::to_string(php::parameters& params) {
        if((status_ & 0x100) == 0) build_ex();
        return c_mail_;
    }
}

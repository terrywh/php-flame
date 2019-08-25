<?php
/**
 * 使用 SMTP 协议进行简单邮件发送
 */
namespace flame\smtp;

/**
 * 创建 SMTP 客户端，请使用 URL 形式定义 SMTP 服务配置：
 * @param string $url 例如：smtp://user:pass@127.0.0.1:25/
 */
function connect($url): flame\smtp\client {
    return new flame\smtp\client();
}
/**
 * SMTP 客户端
 */
class client {
    /**
     * 请使用 flame\smtp\connect 连接/创建客户端
     */
    private function __construct() {}
    /**
     * 发送邮件
     */
    function post(flame\smtp\message $mail) {}
    /**
     * 简化的发送邮件函数：
     * @param string $from 来源邮箱, 形如 "xxxx@xxx.xxx" （不含引号）
     * @param array  $to 目标邮箱，可以使用如下形式：
     *  array(
     *     "名字1"=>"xxxx@xxxx.xxx",
     *     "名字2"=>"xxxx@xxxx.xxx",
     *     ...
     *  );
     * 或
     *  array(
     *     "xxxx@xxxx.xxx",
     *     "xxxx@xxxx.xxx",
     *     ...
     *  );
     * @param string $subject 主题
     * @param string $html 邮件内容，请使用 HTML 形式；
     */
    function send(string $from, array $to, string $subject, string $html) {}
}

class message {
    /**
     * 设置邮件发送方信息
     * @param $from 例如 `array("测试"=>"test@test.com")`, 或 `["test@test.com"]`
     * @return message 消息对象自身，便于串联调用
     * 注意：仅允许设置一次，必须在加入邮件内容前进行; 设置多个来源方信息，仅第一项生效
     */
    function from(array $from):message {
        return $this;
    }
    /**
     * 设置邮件接收方信息
     * @return message 消息对象自身，便于串联调用
     * 注意：仅允许设置一次，必须在加入邮件内容前进行
     * @see send() / $to
     */
    function to(array $to):message {
        return $this;
    }
    /**
     * 设置邮件抄送方信息
     * @return message 消息对象自身，便于串联调用
     * 注意：仅允许设置一次，必须在加入邮件内容前进行
     * @see send() / $to
     */
    function cc(array $cc):message {
        return $this;
    }
    /**
     * 设置主题
     * @return message 消息对象自身，便于串联调用
     * 注意：仅允许设置一次，必须在加入邮件内容前进行
     */
    function subject(string $subject):message {
        return $this;
    }
    /**
     * 添加发送内容（MultiPart)
     * @return message 消息对象自身，便于串联调用
     * 注意：必须在上述 主题/接收者等设置完毕后进行
     * @example 加入 HTML 内容：
     *  $msg->append("<strong>这是 HTML 内容</strong>", ["Content-Type" => "text/html; charset=\"UTF-8\""]);
     * @example 加入 文件 附件：
     *  $msg->append(file_get_contents("/path/to/attachment/abc.png"), ["Content-Type" => "application/octet-stream", "Content-Disposition" => "attachment; filename=\"abc.png\""]);
     * @example 加入 嵌入 附件：
     *  $msg->append(..., [..., "Content-Disposition" => "attachment; filename=\"abc.jpg\"", "Content-ID" => "image_cid_1", ...]);
     *  $msg->append("... <img src=\"cid:image_cid_1\" /> ...", ["Content-Type" => "text/html; charset=\"UTF-8\""]);
     */
    function append(string $data, array $header):message {
        return $this;
    }
    /**
     * 调试，返回生成 MIME 形式的邮件原文
     */
    function __toString() {
        
    }
}
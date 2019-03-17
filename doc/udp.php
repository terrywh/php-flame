<?php
/**
 * 提供对 UDP 网络操作的简单封装
 */
namespace flame\udp;

/**
 * 连接指定地址, 并返回 socket 对象
 * @param string $address 连接地址, 例如: "127.0.0.1:8687"
 * 注意: UDP "链接" 能在一定程度上提高效率;
 */
function connect(string $address):socket {}

class socket {
    /**
     * 构建 socket 可选的进行地址绑定
     * 实际函数拥有两种形式:
     *  __construct(string $address, array $option = [])
     * 或
     *  __construct(array $option = [])
     * 前者可用于地址绑定;
     * @param array $option 可用选项如下:
     * * `max` integer - 接收缓冲区上限(默认为 64k 可减小已提升效率)
     */
    function __construct(mixed $opt_or_address = null, array $option = []) {}
    /**
     * 本地地址（含端口）
     * @property string
     */
    public $local_address;
    /**
     * 远端地址（含端口）
     * @property string
     */
    public $remote_address;
    /**
     * 发送一个 UDP 包给已连接的对端
     * 注意: 只能在"连接"形态下使用
     * @see flame\udp\connect()
     */
    function send(string $data) {}
    /**
     * 向指定目标 $to 发送一个 UDP 包
     * @param string $to 目标地址, 可以是域名或地址;
     */
    function send_to(string $data, string $to) {}
    /**
     * 接收并返回一个 UDP 包
     * @return mixed 返回接收到的数据包(文本 string); 若 socket 被关闭则返回 null;
     */
    function recv():mixed {}
    /**
     * 关闭连接
     * 注意: 正在进行的发送 send(_to) 或 recv(_from) 接收动作将被终止;
     */
    function close() {}
}
/**
 * 简化的服务端, 相较使用上述 socket 自行实现效率稍高
 */
class server {
    /**
     * 创建一个服务器, 绑定在指定的地址上, 准备服务
     * @param string $address 绑定地址, 例如: "0.0.0.0:8888" 或 ":::8888"
     * @param string
     */
    function __construct(string $address, array $options = []) {}
    /**
     * 启动服务器, 当有连接建立时, 启用协程执行回调函数;
     * @param callable $cb 回调函数, 形如:
     *  function(string $data, string $from) {}
     *  $data - 接收的数据包
     *  $from - 来源地址
     */
    function run(callable $cb) {}
    /**
     * 使用当前服务器对应 UDP 套接字, 向指定地址发送消息
     * @see flame::socket::send_to()
     */
    function send_to(string $data, string $address) {}
    /**
     * 关闭服务器 (结束 run() 过程, 保证正在进行处理的"消费"协程完成)
     */
    function close() {}
}

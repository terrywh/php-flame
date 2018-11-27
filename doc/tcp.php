<?php
namespace flame\tcp;

/**
 * 连接指定地址, 并返回 socket 对象
 * @param string $address 连接地址, 例如: "127.0.0.1:8687"
 */
function connect(string $address):socket {}

class socket {
    public $local_address;
    public $remote_address;
    /**
     * 读取一定量的数据, 实际读取方式根据 $completion 参数决定;
     * @param mixed $completion 
     *  * 参数为 null 时, 表示随机读取一定量数据 (不会超过 8192 字节);
     *  * 参数为 int 时, 表示读取指定量的数据, 例如 $completion = 1024 表示读取 1024 字节;
     *  * 参数为 string 时, 表示读取到指定的结束符, 例如 $completion = "\r\n" 读取到 "abc\r\n" 字符串;
     */
    function read(mixed $completion = null) {}
    /**
     * 发送数据
     * @param string $data
     */
    function write(string $data) {}
    /**
     * 关闭连接
     */
    function close() {}
}
class server {
    /**
     * 创建一个服务器, 绑定在指定的地址上, 准备服务
     * @param string $address 绑定地址, 例如: "0.0.0.0:8888" 或 ":::8888"
     */
    function __construct(string $address) {}
    /**
     * 启动服务器, 当有连接建立时, 启用协程执行回调函数;
     * @param callable $cb 回调函数, 形如:
     *  function($socket) {}
     */
    function run(callable $cb) {}
    /**
     * 关闭服务器
     */
    function close() {}
}
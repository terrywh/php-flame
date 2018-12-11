<?php
/**
 * 基本框架流程函数, 用于框架初始化, 启动协程等;
 */
namespace flame;
/**
 * 初始化框架, 设置进程名称及相关配置;
 * @param string $process_name 进程名称
 * @param array  $options 选项配置, 目前可用如下:
 *  * `logger` - 日志输出重定向目标文件(完整路径, 若不提供使用标准输出); 
 *      向主进程发送 SIGUSR2 信号该文件将会被重新打开(或生成);
 *  * `level` - 日志输出级别, 设置该级别下的日志将不被记录; 可用级别如下
 *      "debug"
 *      "info"
 *      "warning"
 *      "error"
 *      "fatal"
 *  请参见 flame\log 命名空间;
 */
function init($process_name, $options = []) {}
/**
 *  启动协程(运行对应回调函数)
 */
function go(callable $cb) {}
/**
 * 框架调度, 上述协程会在框架开始调度运行后启动
 */
function run() {}
/**
 * 从若干个队列中选择(等待)一个有数据队列
 * @return 若所有通道已关闭, 返回 null; 否则返回一个有数据的通道, 即: 可以无等待 pop()
 */
function select(channel $q1, $q2/*, ...*/):channel {}
/**
 * 监听框架的通知
 * @param string $event 目前消息存在以下两种:
 *  * "exception" - 未捕获的异常通知;
 *  * "quit" - 退出消息, 一般用于平滑停止各种服务;
 * @param callable 回调函数
 */
function on(string $event, callable $cb) {}
/**
 * 用于在用户异常处理流程中退出整个框架
 */
function quit() {}
/**
 * 协程型队列
 */
class queue {
    /**
     * @param int $max 队列容量, 若已放入数据达到此数量, push() 将"阻塞"(等待消费);
     */
    function __construct($max = 1) {}
    /**
     * 放入; 若向已关闭的队列放入, 将抛出异常;
     */
    function push($v) {}
    /**
     * 取出
     */
    function pop() {}
    /**
     * 关闭 (将唤醒阻塞在取出 pop() 的协程);
     * 原则上仅能在生产者方向关闭队列;
     */
    function close() {}
}

class mutex {
    /**
     * 构建一个协程式 mutex 对象
     */
    function __construct() {}
    /**
     * 加锁
     */
    function lock() {}
    /**
     * 解锁
     */
    function unlock() {}
}

class guard {
    /**
     * 构建守护并锁定 mutex 
     * @param mutex $mutex 实际保护使用的 mutex 对象
     */
    function __construct(mutex $mutex) {}
    /**
     * 解锁 mutex 并销毁守护
     */
    function __destruct() {}
}
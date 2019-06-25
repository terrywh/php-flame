<?php
/**
 * 基本框架流程函数, 用于框架初始化, 启动协程等;
 * 框架可在两种模式下工作：
 * 
 * 1. 单进程模式：
 *  * 一次信号 SIGINT / SIGTERM 将 “通知” 进程退出（等待超时），第二次或超时后立即停止；
 *  * 信号 SIGUSR1 将切换 HTTP 服务器长短连状态 `Connection: close`；
 *  * 单进程模式**不支持**日志文件输出，固定输出到 STDERR / STDOUT (取决于错误等级）；
 * 
 * 2. 多进程模式：
 *  * 使用环境变量 FLAME_MAX_WORKERS=X 启动多进程模式；
 *  * 主进程将自动进行日志文件写入，子进程自动拉起；
 *  * 端口监听使用 SO_REUSEPORT 在多个工作进程间共享，但不排除外部监听的可能；
 *  * 向主进程发送 SIGINT / SIGTERM 一次将 ”通知“ 子进程退出（等待超时），第二次或超时后将立即停止；
 *  * 向主进程发送 SIGUSR2 信号该文件将会被重新打开(或生成);
 *  * 子进程继承主进程的 环境变量 / -c /path/to/php.ini 配置（通过命令行 -d XXX=XXX 的临时设置无效）；
 *  * 向主进程发送 SIGUSR1 信号将切换 HTTP 服务器长短连状态 `Connection: close`；
 *  * 向主进程发送 SIGRTMIN+1 将进行进程重载（陆续停止当前进程并启动新进程）
 */
namespace flame;
/**
 * 初始化框架, 设置进程名称及相关配置;
 * @param string $process_name 进程名称
 * @param array  $options 选项配置, 目前可用如下:
 *  * "logger" - 日志输出重定向目标文件 (完整路径, 若不提供使用标准输出，单进程模式无效);
 *  * "level" - 日志输出级别, 设置该级别下的日志将不被输出记录; 可用级别如下
 *      "debug"
 *      "info"
 *      "warning"
 *      "error"
 *      "fatal"
 *  * "timeout" - 多进程退出超时, 单位毫秒 ( 200 ~ 100000ms ), 默认 3000ms（超时后会被强制杀死）
 *  @see flame\log
 */
function init($process_name, $options = []) {}
/**
 * 启动协程(运行对应回调函数)
 * 注意：框架使用 固定栈大小 方式进行协程调度（相对效率较高）；但为满足大部分需求，栈空间占用相对（初始值）较高；不应启动过多的协程；
 */
function go(callable $cb) {}
/**
 * 读取数组的指定层级键（常用于读取配置数据）
 * @example
 *  $a = ["a" => ["b" => 123]];
 *  var_dump(flame\get($a, "a.b")); // 123
 * @return mixed
 * 注意：
 *  1. 数字下标将被当作文本处理；
 */
function get($array, $keys) {}
/**
 * 设置数组的层级键值
 * @example
 *  $a = [];
 *  flame\set($a, "a.b", 123);
 *  var_dump($a); // ["a" => ["b" => 123]]
 * 注意：
 *   1. 数字下标将被当作文本处理；
 */
function set(&$array, $keys, $val) {}
/**
 * 获取一个当前协程 ID 标识
 */
function co_id():int {
    return 123456;
}
/**
 * 获取运行中的协程数量
 */
function co_count():int {
    return 8;
}
/**
 * 框架调度, 上述协程会在框架开始调度运行后启动
 * 注意：协程异步调度需要 run() 才能启动执行；
 */
function run() {}
/**
 * 从若干个队列中选择(等待)一个有数据队列
 * @return 若所有通道已关闭, 返回 null; 否则返回一个有数据的通道, 即: 可以无等待 pop()
 */
function select(queue $q1, $q2/*, ...*/):queue {
    return new queue();
}
/**
 * 监听框架的通知
 * @param string $event 目前消息存在以下两种:
 *  * "exception" - 当协程发生未捕获异常, 执行对应的回调，并记录错误信息（随后进程会退出），用户可在此回调进行错误报告或报警;
 *  * "quit" - 退出消息, 用户可在此回调停止各种服务，如停止 HTTP 服务器 / 关闭 MySQL 连接等;
 * @param callable 回调函数
 */
function on(string $event, callable $cb) {}
/**
 * 用于在用户处理流程中退出
 * 注意：
 * 使用 PHP 内置 exit() 进行退出可能导致框架提供的清理、退出机制无效；
 * 请示用本函数代替；
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
    /**
     * 队列是否已关闭
     */
    function is_closed(): bool {
        return false;
    }
}
/**
 * 协程互斥量（锁）
 */
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
/**
 * 协程锁（使用上述 mutex 构建自动加锁解锁流程）
 */
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

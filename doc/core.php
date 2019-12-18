<?php
/**
 * 基本框架流程函数, 用于框架初始化, 启动协程等;
 * 框架可在两种模式下工作：
 * 
 * 1. 单进程模式：
 *  * 信号 SIGQUIT 立即终止进程；
 *  * 信号 SIGINT / SIGTERM 将 “通知” 进程退出 (触发 `quit` 回调)，等待超时后立即终止；
 *  * 信号 SIGUSR1 将切换 HTTP 服务器长短连状态 `Connection: close`；
 *  * 信号 SIGUSR2 日志文件将会被重新打开(或生成);
 * 
 * 2. 父子进程模式：
 *  * 使用环境变量 FLAME_MAX_WORKERS=X 启动父子进程模式；
 *  * 子进程存在环境变量 FLAME_CUR_WORKER=N 标识该进程标号，1 ~ FLAME_MAX_WORKERS
 *  * 主进程将自动进行日志文件写入，子进程自动拉起；
 *  * 端口监听使用 SO_REUSEPORT 在多个工作进程间共享，但不排除外部监听的可能；
 * 
 *  * 向主进程发送 SIGINT / SIGQUIT 将立即终止进程；
 *  * 项主进程发送 SIGTERM 将 ”通知“ 子进程退出 (触发 `quit` 回调)，并等待超时后立即终止；
 *  * 向主进程发送 SIGUSR2 信号该文件将会被重新打开(或生成);
 *  * 子进程继承主进程的 环境变量 / -c /path/to/php.ini 配置（通过命令行 -d XXX=XXX 的临时设置无效）；
 *  * 向主进程发送 SIGUSR1 信号将切换 HTTP 服务器长短连状态 `Connection: close`；
 *  * 向主进程发送 SIGRTMIN+1 将进行进程重载（陆续平滑停止当前工作进程并重新启动）
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
 *  * "timeout" - 进程退出超时, 单位毫秒 ( 200 ~ 100000ms ), 默认 3000ms（超时后会被强制杀死）
 *  @see flame\log
 */
function init($process_name, $options = []) {}
/**
 * 启动协程(运行对应回调函数)
 * 注意：框架使用 固定栈大小 方式进行协程调度（相对效率较高）；但为满足大部分需求，栈空间占用相对（初始值）较高；不应启动过多的协程；
 */
function go(callable $cb) {}
/**
 * 框架启动、调度运行
 * @param callable $cb 可选，启动一个协程（同 `flame\go()` 函数）
 * @see flame\go()
 * 注意：此函数阻塞并持续运行直到所有协程结束；
 */
function run(callable $cb = null) {}
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
 * 从若干个队列中选择(等待)一个有数据队列
 * @return 若所有通道已关闭, 返回 null; 否则返回一个有数据的通道, 即: 可以无等待 pop()
 */
function select(queue $q1, $q2/*, ...*/):queue {
    return new queue();
}
/**
 * 向指定工作进程发送消息通知
 * @param int $target_worker 目标工作进程编号，1 ~ FLAME_MAX_WORKERS；当前进程可读取环境变量 FLAME_CUR_WORKER 获取编号；
 * @param mixed $data 消息数据，自动进行 JSON 序列化传输；
 * 注意：
 *   对象类型数据进行 JSON 序列化可能丢失数据细节而无法还原；
 *   目标进程将会收到 `message`（自动 JSON 反序列化数据）回调;
 *   @see flame\on("message", $cb);
 */
function send(int $target_worker, $data) {

}
/**
 * 为指定事件添加处理回调（函数）
 * @param string $event 目前消息存在以下两种:
 *  * "exception" - 当协程发生未捕获异常, 执行对应的回调，并记录错误信息（随后进程会退出），用户可在此回调进行错误报告或报警;
 *  * "quit" - 退出消息, 用户可在此回调停止各种服务，如停止 HTTP 服务器 / 关闭 MySQL 连接等;
 *  * "message" - 消息通知，（启动内部协程）接收来自各子进程之间的相互通讯数据；回调函数接收一个不限类型参数；
 * @see flame\send()
 * @param callable 回调函数
 */
function on(string $event, callable $cb) {}
/**
 * 删除指定事件的所有处理回调（函数）
 * 注意：
 *  由于 `message` 事件内部启动协程，阻止了程序的停止；须取消对应回调，使该内部协程停止后，进程才可以正常退出；
 */
function off(string $event) {}
/**
 * 用于在用户处理流程中退出
 * 注意：
 *  1. 使用 PHP 内置 exit() 进行退出可能导致框架提供的清理、退出机制无效；(请示用本函数代替)
 *  2. 调用本函数主动退出进程，不会触发上述 `on("quit", $cb)` 注册的回调;
 *  3. 父子进程模式，非停止流程，当 $exit_code 不为 0 时表示异常退出，父进程会（1s ~ 3s 后）将当前工作进程重启；
 */
function quit(int $exit_code = 0) {}
/**
 * 生成一个 唯一编号 (兼容 SNOWFLAKE 格式: https://github.com/bwmarrin/snowflake)
 * @param int $node 节点编号 (须为不同服务器、进程设置不同值 0 ~ 1023 范围)
 * @param int $epoch 参照时间 (毫秒，生成相对时间戳 1000000000000 ~ CURRENT_TIME_MS 范围)
 * 注意：
 *   1. 配置不同的参照时间 `$epoch` 可以生成不同号段；
 */
function unique_id(int $node, int $epoch = 1288834974657):int {
    static $inc = 0;
    return (flame\time\now() - $epoch) << 22 | $node << 12 | $inc++;
}
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

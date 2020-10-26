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
 * 生成一个 唯一ID (兼容 SNOWFLAKE 格式: https://github.com/bwmarrin/snowflake)
 * @param int $node 节点编号 (须为不同服务器、进程设置不同值 0 ~ 1023 范围，默认 PID % 1024)
 * @param int $epoch 参照时间 (毫秒，生成相对时间戳 1000000000000 ~ CURRENT_TIME_MS 范围)
 * 注意：
 *   1. 配置不同的参照时间 `$epoch` 可以生成不同号段；
 */
function unique_id(int $node = 0, int $epoch = 1288834974657):int {
    static $inc = 0;
    return (flame\time\now() - $epoch) << 22 | $node << 12 | $inc++;
}
/**
 * 记录 TRACE 级别日志, 自动进行 JSON 形式的序列化;
 */
function trace($x/*, $y, $z  ... */) {}
/**
 * 记录 DEBUG 级别日志, 自动进行 JSON 形式的序列化;
 */
function debug($x/*, $y, $z  ... */) {}
/**
 * 记录 INFO 级别日志, 自动进行 JSON 形式的序列化;
 */
function info($x/*, $y, $z  ... */) {}
/**
 * 记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
 */
function warn($x/*, $y, $z  ... */) {}
/**
 * 记录 ERROR 级别日志, 自动进行 JSON 形式的序列化;
 */
function error($x/*, $y, $z  ... */) {}
/**
 * 记录 FATAL 级别日志, 自动进行 JSON 形式的序列化;
 */
function fatal($x/*, $y, $z  ... */) {}

class logger {
    /**
     * 禁止手动创建日志对象，请使用 flame\log\connect() 函数
     * @see flame\log\connect()
     */
    private function __construct() {}
    /**
     * 记录 无前缀（无时间戳 、无警告级别）的日志数据
     */
    function write($x/*, $y, $z  ... */) {}
    /**
     * 记录 TRACE 级别日志, 自动进行 JSON 形式的序列化;
     */
    function trace($x/*, $y, $z  ... */) {}
    /**
     * 记录 DEBUG 级别日志, 自动进行 JSON 形式的序列化;
     */
    function debug($x/*, $y, $z  ... */) {}
    /**
     * 记录 INFO 级别日志, 自动进行 JSON 形式的序列化;
     */
    function info($x/*, $y, $z  ... */) {}
    /**
     * 记录 WARNING 级别日志, 自动进行 JSON 形式的序列化;
     */
    function warn($x/*, $y, $z  ... */) {}
    /**
     * 记录 ERROR 级别日志, 自动进行 JSON 形式的序列化;
     */
    function error($x/*, $y, $z  ... */) {}
    /**
     * 记录 FATAL 级别日志, 自动进行 JSON 形式的序列化;
     */
    function fatal($x/*, $y, $z  ... */) {}
};

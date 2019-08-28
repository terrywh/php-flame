<?php
/**
 * 时间相关函数
 */
namespace flame\time;

/**
 * 当前协程休眠 $ms 毫秒（调度式休眠）
 */
function sleep(int $ms) {}
/**
 * 经过若干时间执行回调
 * 注意：
 *    1. 相较大量自行启动协程使用 `sleep` 方式的定时器，资源占用稍低（使用单一底层定时器 + 优先级队列统一调度）；
 * @see timer
 */
function after(int $ms, callable $cb) {}
/**
 * 每隔若干时间执行回调（自动计算 $cb 实际执行时间并尝试校准）
 * @param callable $cb 回调函数，当回调函数返回 false 时，将不再被调用（相当于关闭定时器 `$tm->close()` 被调用）
 * 注意：
 *    1. 相较大量自行启动协程使用 `sleep` 方式的定时器，资源占用稍低（使用单一底层定时器 + 优先级队列统一调度）；
 * @see timer
 */
function tick(int $ms, callable $cb) {}
/**
 * 底层定时器的包裹类型，提供对底层定时器的控制接口
 * 注意：
 *   1. 指定的回调将在定时器独立的协程中被调用执行；
 *   2. 当回调函数返回 false 时，自动关闭定时器（相当于 `$tm->close()` 被调用）；
 *   3. 除非定时器被关闭，即使当前 `timer` 定时器对象被销毁，定时器回调也会执行；
 */
class timer {
    /**
     * 创建定时器
     * @param int $interval 间隔（等待）时间，单位为 毫秒/ms
     * @param mixed $cb 可选，指定定时器回调；若类型非 callable 则标识重复设置，同 $repeat;
     * @param bool $repeat 可选，默认 true 指定是否创建反复执行的定时器；
     */
    function __construct(int $interval, $cb = null, bool $repeat = true) {}
    /**
     * 启动定时器（回调将在指定的时间被调用）
     * @param callable $cb 可选，指定定时器回调（覆盖/重新指定）
     */
    function start(callable $cb = null) {}
    /**
     * 关闭（销毁）定时器（回调不会再被执行）
     */
    function close() {}
}
/**
 * 获取毫秒时间戳
 * (此时间戳采用框架缓存机制，较默认获取系统时间函数消耗较小)
 */
function now():int {
    return 1557659996427;
}
/**
 * 获取标准时间 "YYYY-mm-dd HH:ii:ss"
 * (按标准格式输出上述 `now()` 对应时间)
 */
function iso():string {
    return "2019-03-31 21:40:25";
}

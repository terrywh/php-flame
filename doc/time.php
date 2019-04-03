<?php
/**
 * 时间相关函数
 */
namespace flame\time;

/**
 * 当前协程休眠 $ms 毫秒
 */
function sleep(int $ms) {}
/**
 * 获取毫秒时间戳
 * (此时间戳采用框架缓存机制，较默认获取系统时间函数消耗较小)
 */
function now():int {
    return intval(microtime(true) * 1000);
}
/**
 * 获取标准时间 "YYYY-mm-dd HH:ii:ss"
 * (按标准格式输出上述 `now()` 对应时间)
 */
function iso():string {
    return "2019-03-31 21:40:25";
}

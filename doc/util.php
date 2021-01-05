<?php
/**
 * 其他
 */
namespace flame\util;

/**
 * 一次性生成唯一标识 (兼容 SNOWFLAKE 格式: https://github.com/bwmarrin/snowflake)
 * @param int $node 节点编号，可选 1 ~ 1023 范围；
 * @param int $epoch 参照时间 (毫秒，生成相对时间戳 1000000000000 ~ CURRENT_TIME_MS 范围)
 * @return int
 * 注意：
 *   1. 服务应为不同服务器、进程设置不同的 节点编号值（），默认使用当前进程 PID 取模；
 *   2. 设置不同的参照时间可控制生成不同的范围；
 *   3. 多次、重复生成唯一ID，应考虑使用类 `flame\util\snowflake` 代替；
 */
function unique_id(int $node = null, int $epoch = 1288834974657) {}
/**
 * 封装提供一个唯一标识的 snowflake 兼容算法
 */
class snowflake {
    /**
     * @param int $node 节点编号，可选 1 ~ 1023 范围；
     * @param int $epoch 参照时间 (毫秒，生成相对时间戳 1000000000000 ~ CURRENT_TIME_MS 范围)
     */
    function __construct(int $node = null, int $epoch = 1288834974657) {}
    /**
     * 生成/获取下一个唯一标识
     * @return int
     * 此方法开销很低，一般无需考虑批量获取；
     * 
     */
    function next_id() {}
}

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


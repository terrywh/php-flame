<?php
/**
 * 提供 Redis 基本客户端封装，内部使用连接池：
 * 注意：
 * 1. 暂未实现 SUBSCRIBE 相关命令；
 * 2. 单客户端内连接池大小上限为 3 （单进程）；
 */
namespace flame\redis;

function connect(string $url):client {}
/**
 * Redis 客户端
 */
class client {
    /**
     * 用于通用的实现可用 Redis 命令，支持 REDIS 大部分指令
     * @return mixed 可能返回 int/string/array/null 等类型
     */
    function __call($name, $argv) {}
    /**
     * @return bool true
     */
    function __isset($name) {
        return true;
    }
    /**
     * 支持 Redis 大部分指令:
     *
     * @method get()
     * @method set()
     * @method incr()
     * @method zincr()
     * @method mset()
     * @method mget()
     * @method hmset()
     * @method hmget()
     * @method hgetall()
     * @method zrange()
     * @method zrevrange()
     * @method zrangebyscore()
     * @method zrevrangebyscore()
     * ...
     * 这里不再一一列举
     */
    /**
     * 批量执行
     */
    function multi(): tx {}
    /**
     * @uimplement 暂未实现
     */
    function subscribe() {}
    /**
     * @uimplement 暂未实现
     */
    function unsubscribe() {}
    /**
     * @uimplement 暂未实现
     */
    function psubscribe() {}
    /**
     * @uimplement 暂未实现
     */
    function punsubscribe() {}
}
/**
 * 用于记录待执行的命令并批量执行
 */
class tx {
    /**
     * 所有 client 支持的命令, 但不会立即执行; 返回对象自身, 可连续串写, 例如:
     *  @example
     *      $r = $tx->get("xxx")->set("xxx","yyy")->exec();
     */
    function __call($name, $argv): tx {
        return $this;
    }
    /**
     * 执行目前提交的命令并带回所有响应返回
     * @return array 返回数据项与命令一一对应
     */
    function exec():array {}
}

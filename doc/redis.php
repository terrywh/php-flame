<?php
/**
 * MongoDB 基本客户端
 */
namespace flame\redis;

function connect(string $url):client {}
/**
 * 
 */
class client {
    /**
     * 用于通用的实现可用 Redis 命令
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
 * 
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
     */
    function exec():array {}
}
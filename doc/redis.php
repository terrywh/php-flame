<?php
/**
 * 提供 Redis 基本客户端封装，内部使用连接池：
 * 注意：
 * 1. 暂未实现 SUBSCRIBE 相关命令；
 * 2. 单客户端内连接池大小上限为 6 （单进程），即访问并行很大时若 6 个连接都在使用中，须排队等待；
 */
namespace flame\redis;

function connect(string $url):client {
    return new client();
}
/**
 * Redis 客户端
 * 支持 Redis 大部分指令:
 *
 * @method get(string $key)
 * @method int del(string $key)
 * @method int expire(string $key, int $seconds)
 * @method int ttl(string $key)
 * @method string type(string $key)
 * @method string set(string $key, string $val)
 * @method string mset(string $key, string $val)
 * @method array mget(string $key)
 * @method array scan(int $cursor)
 * @method int incr(string $key)
 * @method int incrby(string $key, int $by)
 * @method int sadd(string $key, string $member)
 * @method int scard(string $key)
 * @method string spop(string $key)
 * @method int srem(string $key, string $member)
 * @method string lpop(string $key)
 * @method string rpop(string $key)
 * @method int llen(string $key)
 * @method int lpush(string $key)
 * @method int rpush(string $key)
 * @method array blpop(string $key)
 * @method array lrpop(string $key)
 * @method int hdel(string $key)
 * @method hget(string $key, string $field)
 * @method int hset(string $key, string $field, string $val)
 * @method array hmget(string $key, string $field1)
 * @method string hmset(string $key, string $field, string $val)
 * @method array hgetall(string $key)
 * @method int hincrby(string $key, string $field, int $by)
 * @method int hlen(string $key)
 * @method int zadd(string $key, $score, $member)
 * @method int zincrby(string $key, int $inc, string $member)
 * @method int zcount(string $key, int $min, int $max)
 * @method int zcard(string $key)
 * @method int zrank(string $key, string $member)
 * @method int zrem(string $key, string $member)
 * @method string zscore(string $key, string $member)
 * @method array zrange(string $key, int $start, int $stop)
 * @method array zrevrange(string $key, int $start, int $stop)
 * @method array zrangebyscore(string $key, int $min, int $max)
 * @method array zrevrangebyscore(string $key, int $min, int $max)
 * ...
 * 这里仅列出了常用的部分函数及其对应的必要参数
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
     * 批量执行
     */
    function multi(): tx {
        return new tx();
    }
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
    function exec():array {
        return [];
    }
}



$r = new client();

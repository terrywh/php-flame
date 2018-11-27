<?php

namespace flame\rabbitmq;

/**
 * 通过 RabbitMQ 连接地址连接并准备消费
 * @param string $url 连接地址, 形如:
 *  amqp://{username}:{password}@{host}:{port}/{vhost}
 */
function consume(string $url, string $queue): consumer {}
/**
 * 通过 RabbitMQ 连接地址连接并启动生产
 */
function produce(string $url): producer {}

/**
 * 生产者
 */
class consumer {
    /**
     * 开始消费, 框架将按照一定数量启动并行消费协程, 并使用消息对象调用 $cb 回调函数;
     * @param callable $cb 消费回调, 形如:
     *  function callback($message) {}
     */
    function run(callable $cb) {}
    /**
     * 确认消息
     */
    function confirm(message $msg) {}
    /**
     * 拒绝消息(可选的返回队列, 重复消费)
     */
    function reject(message $msg, bool $requeue = false) {}
    /**
     * 关闭消费者 (结束 run() 执行流程)
     * 注意: 由于消费速度较快, 关闭消费者可能需要一段时间, 以保证完整消费;
     */
    function close() {}
}
class producer {
    /**
     * @param mixed $message 为 string 时表示生产的消息的包体; 也可以为 message 类型的对象;
     * @param string $routing_key 路由KEY, 可以覆盖 $message 对象内部的 routing_key 属性
     */
    function publish(string $exchange, mixed $message, string $routing_key = null) {}
}
class message implements JsonSerializable {
    public $routing_key;
    public $body;
    public $expiration;
    public $reply_to;
    public $correlation_id;
    public $priority;
    public $delivery_mode;
    /**
     * @property array
     */
    public $header;
    public $content_encoding;
    public $content_type;
    public $cluster_id;
    public $app_id;
    public $user_id;
    public $type_name;
    public $timestamp;
    public $message_id;
    /**
     * 构建一条新的 message 消息
     */
    function __construct($body = "", $routing_key = "") {}
    function __toString() {}
    /**
     * 自定义 JSON 序列化, 返回 routing_key / body / timestamp 字段;
     */
    function jsonSerialize() {}
}

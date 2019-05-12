<?php
/**
 * 提供简单的 RabbitMQ 生产消费封装
 */
namespace flame\rabbitmq;

/**
 * 通过 RabbitMQ 连接地址连接并准备消费
 * @param string $url 连接地址, 形如:
 *  amqp://{username}:{password}@{host}:{port}/{vhost}?key1=val1
 * 可使用参数:
 *  * `prefetch` - int - 用于指定未 ack 时最大读取数量
 */
function consume(string $url, string $queue): consumer {
    return new consumer();
}
/**
 * 通过 RabbitMQ 连接地址连接并启动生产
 */
function produce(string $url): producer {
    return new producer();
}

/**
 * 生产者
 */
class consumer {
    /**
     * 开始消费, 框架将按照一定数量启动并行消费协程, 并使用消息对象调用 $cb 回调函数;
     * @param callable $cb 消费回调, 形如:
     *  function callback($message) {}
     * 当消费过程失败，将抛出异常（结束消费）
     */
    function run(callable $cb) {}
    /**
     * 确认消息
     */
    function confirm(message $msg) {}
    /**
     * 拒绝消息(可选的返回队列, 重复消费)
     * @param bool $requeue 若为真值，将被拒绝的消息放回队列重新等待消费；
     */
    function reject(message $msg, bool $requeue = false) {}
    /**
     * 关闭消费者 (结束 run() 执行流程)
     * 注意: 由于消费速度较快, 关闭消费者可能需要一段时间, 以保证完整消费;
     */
    function close() {}
}
/**
 * 消费者
 */
class producer {
    /**
     * 通过指定 exchange 生产（发送）一条消息
     * @param mixed $message 为 string 时表示生产的消息的包体; 也可以为 message 类型的对象;
     * @param string $routing_key 路由KEY, 可以覆盖 $message 对象内部的 routing_key 属性
     */
    function publish(string $exchange, $message, string $routing_key = null) {}
}
/**
 * 消息对象
 */
class message implements JsonSerializable {
    /**
     * @var string 路由键值
     */
    public $routing_key;
    /**
     * @var string 消息体
     */
    public $body;
    public $expiration;
    public $reply_to;
    public $correlation_id;
    public $priority;
    public $delivery_mode;
    /**
     * @var array
     */
    public $header;
    public $content_encoding;
    public $content_type;
    public $cluster_id;
    public $app_id;
    public $user_id;
    public $type_name;
    /**
     * @var int 消息时间戳，一般为秒级（由于可用户自定指定，含义可能发生变异）
     */
    public $timestamp;
    public $message_id;
    /**
     * 构建一条新的 message 消息
     */
    function __construct(string $body = "", string $routing_key = "") {}
    /**
     * 返回消息体
     */
    function __toString(): string {
        return "body of message";
    }
    /**
     * 自定义 JSON 序列化, 返回包含 routing_key / body / timestamp 字段的数组;
     */
    function jsonSerialize(): array {
        return [];
    }
}

<?php
/**
 * 提供 Kafka 基本生产消费功能封装
 */
namespace flame\kafka;
/**
 * @param array $config 基本 Kafka 配置, 以下两个选项必要:
 *  * "bootstrap.servers" - string 服务器地址(9092);
 *  * "group.id" - string 消费组名称(同一消费组共享消费进度);
 * 可以使用一个自定义的的参数:
 *  * "concurrent" - 控制并行消费协程数量, 默认为 8 (数值类型)，上限为 256;
 * @see 其他可配置选项均为字符串类型(非字符串会被强制转换), 请参考:
 *  https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md
 * @param array $topics 待消费的数据源 TOPIC 名称列表
 */
function consume(array $config, array $topics) {}

/**
 * @param array $config 基本 Kafka 配置, 一下两个选项必要:
 *  * "bootstrap.servers" - 服务器地址(9092);
 *  * "group.id" - 消费组名称(同一消费组共享消费进度);
 * @see 其他可配置选项请参考:
 *  https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md
 * @param array $topics 待生产的目标 TOPIC 名称列表
 */
function produce(array $config, array $topics) {}
/**
 * 消费者
 */
class consumer {
    /**
     * 执行消费, 实际的回调函数会被对应的协程回调执行
     * 注意: 由于 Kafka 消费组的平衡调度, 消费可能会被延迟执行 (15s+);
     * @param callable $cb 消费回调函数
     */
    function run(callable $cb) {}
    /**
     * 手动提交消息(的偏移), 一般需要将 "enable.auto.commit" 设为 "false"
     * @see consume();
     */
    function commit(message $msg) {}
    /**
     * 关闭消费者(停止消费)
     * 注意: 为保证消费消息不丢失, 消费者的实际关闭可能会持续一段时间
     */
    function close() {}
}
/**
 * 生产者
 */
class producer {
    /**
     * 生产消息
     * 注意: 此函数会将消息放入"生产队列", 并通过后台线程传输;
     * (为保证完成, 请使用 `flush()`)
     * @see producer::flush()
     * @param string $topic 生成目标 TOPIC 名称, 必须是 待生产 $topics 数组中的一个;
     * @see produce()
     * @param mixed $mssage 若为 message 类型的对象, 则后续参数无效;
     *  否则为现场 message 对象的 payload 数据;
     * @see class message;
     * @param string $key
     * @param array $header
     */
    function publish(string $topic, mixed $message, string $key = null, array $header = []) {}
    /**
     * 清空"生产队列", 若当前队列中还有消息等待其传输完毕;
     */
    function flush() {}
}
/**
 * 消息对象
 */
class message implements \JsonSerializable {
    /**
     * @property string $topic
     */
    public $topic;
    /**
     * @property int $partition
     */
    public $partition;
    /**
     * @property string $key
     */
    public $key;
    /**
     * @property int $offset
     */
    public $offset;
    /**
     * @property array $header
     */
    public $header;
    /**
     * @property string $payload 消息内容
     */
    public $payload;
    /**
     * @property $timestamp 消息时间戳(一般为毫秒)
     */
    public $timestamp;
    /**
     * 构造一个新消息, 将其 $timestamp 填充为当前时间
     */
    function __construct($payload = "", $key = null) {}
    /**
     * 返回当前消息内容 ($payload)
     */
    function __toString() {}
    /**
     * Json 序列化, 返回 topic + key + payload 关联数组
     */
    function jsonSerialize() {}
}

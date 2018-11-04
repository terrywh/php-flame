## `namespace flame\kafka`

提供 Kafka 协程式客户端封装；**暂不支持**除生产消费以外的功能 (例如: 创建 TOPIC，查看 META 等), 请使用 Kafka 相关管理工具操作;

**示例**：
``` PHP
// 消费
$consumer = yield flame\kafka\consume([
    "bootstrap.servers" => "127.0.0.1:9092,127.0.0.2:9092", // 服务器
    "group.id": "test_consumer_1", // 消费组
], ["topic1", "topic2"]);
flame\time\after(5000, function() use($consumer) {
	// 在另外的协程中终止消费
	$consumer->close();
});
// 持续消费()
yield $consumer->run(function($message) use($consumer) {
    echo $message->header["A"], "\n";
	assert($message->payload == $message->__toString());
    // 可选，手动提交时使用
	// $consumer->commit($message);
});

// 生产
$producer = yield flame\kafka\produce([
    "bootstrap.servers" => "127.0.0.1:9092,127.0.0.2:9092", // 服务器
], ["topic1", "topic2"]);
// 生产
$producer = yield $producer->publish("topic1", "this is as message payload", "this is the key");
// 属性消息生产
$message = new flame\kafka\message("this is another message payload", "this is the key");
$message->header["aaaaaaa"] = "bbbbbbbb";
yield $producer->publish("topic1", $message);
// 消费

```

### `yield flame\kafka\consume(array $config, array $topic) -> flame\kafka\consumer`
连接 `Kafka` 服务器, 建立消费组，准备开始消费; 以下两个 `$config` 选项必须填写：
* `bootstrap.servers` / `metadata.broker.list` - `String` - `Kafka` `broker` 服务器列表（逗号分隔）;
* `group.id` - `String` - 消费组名称（同一消费组共享消费进度）；
其他可用选项请参考：[CONFIGURATION.md](https://github.com/edenhill/librdkafka/blob/v0.11.6/CONFIGURATION.md)

待消费 `$topic` 可指定多个；


### `yield flame\rabbitmq\produce(array $config, array $topic) -> flame\rabbitmq\producer`
连接 `Kafka` 服务器, 准备开始生产消息；以下 `$config` 选项必须填写：
* `bootstrap.servers` / `metadata.broker.list` - `String` - `Kafka` `broker` 服务器列表（逗号分隔）;
其他可用选项请参考：[CONFIGURATION.md](https://github.com/edenhill/librdkafka/blob/v0.11.6/CONFIGURATION.md)

实际能够作为生产目标的 `$topic` 可指定多个；

### `class flame\kafka\consumer`
消费者

#### `yield consumer::run(callable $cb) -> void`
启动消费过程并持续消费, 调用 `$cb` 消费回调协程; 消费协程函数圆形如下:
``` PHP
void callback(flame\kafka\message $message) {}
```

**注意**:
* 运行消费会阻塞当前协程, 可使用 `close()` 停止消费 (恢复当前协程);

#### `boolean consumer::commit(flame\kafka\message $message)`
提交消息偏移；默认状态 Kafka 会自动提交偏移，使用此函数会大幅影响消费效率;

#### `yield consumer::close() -> void`
停止消费者运行过程 (恢复被 `yield run()` 暂停的协程), 不再调用消息回调;

### `class flame\kafka\producer`
生产者

#### `yield producer::publish(string $topic, string $payload[, string $key[, array $header]]) -> void`
向 `$topic` 生产一条以 `$payload` 为内容，`$key` 为键，`$header` 为头信息的消息；

#### `yield producer::publish(string $topic, flame\kafka\message $message[, string $routing_key])`
向 `$topic` 生产一条以 `flame\kafka\message` 类型实例定义的消息;

#### `yield producer::flush() -> void`
等待还在传输中的消息，以确保所有生产消息传输完成；

**注意**：
* 在 `flame\kafka\producer` 对象销毁时也会进行上述等待（最高 10s 时间）；

### `class flame\db\rabbitmq\message`
消费生产过程中使用的消息对象，用于包裹实际的消息内容和其相关属性头信息等数据;

#### `message::__construct([string $payload, [string $key]])`
构建一个 `RabbitMQ` 消息, 可选的填充 `message::$payload` 内容及设定 `message::$key`;

#### `Integer message::$offset`
消息偏移；

#### `String message::$topic`
消费消息时，实际消息的来源 `topic`；

#### `String message::$key`
消息生产时指定的 ROUTING KEY；

#### `String message::$payload`
消息体，内容；

#### `Array message::$header`
消息头（需要 Kafka 0.11.0 以上版本）

#### `Integer message::$timestamp`
消息时间戳（毫秒）；

#### `string message::__toString()`
方便消息处理, 返回消息体，与 `$payload` 一致；

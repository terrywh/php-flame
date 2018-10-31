## `namespace flame\rabbitmq`

提供 RabbitMQ 协程式客户端封装；**暂不支持**除生产消费以外的功能 (例如, declare queue/exchange bind queue 等), 请使用 RabbitMQ UI 界面做手工操作;

**示例**：
``` PHP
// ...
$producer = yield flame\rabbitmq\produce("amqp://user:pass@127.0.0.1:5672/vhost");
// 生产
$producer = $producer->publish("exchange", "this is as message payload", "this is the routing key");
// 属性消息生产
$message = new flame\rabbitmq\message("this is another message payload", "this is the routing key");
$message->user_id = "aaaaaa";
$message->header["aaaaaaa"] = "bbbbbbbb";
$producer->publish("exchange", $message);
// 消费
$consumer = yield flame\rabbitmq\consume("amqp://user:pass@127.0.0.1:5672/vhost", "queue_name_1");
flame\time\after(5000, function() use($consumer) {
	// 在另外的协程中终止消费
	$consumer->close();
});
// 持续消费()
yield $consumer->run(function($message) use($consumer) {
	assert($message->payload == $message->__toString());
	$consumer->confirm($message);
});
```

### `yield flame\rabbitmq\consume(string $url[, string $queue [, array $options = array()]]) -> flame\rabbitmq\consumer`
连接 `RabbitMQ` 服务器, 打开通道, 准备消费指定的队列; 可用选项如下:
* `prefetch` - `Integer` - `RabbitMQ` 预读取数量, 0 < `prefetch` < 65536, 默认 `1`;
* `nolocal` - `Boolean` - 默认 `false`
* `noack` - `Boolean` - 默认 `false`, 消息需确认消费成功, 否则无需确认;
* `exclusive` - `Boolean` - 默认 `false`

**注意**:
* 设置 `prefetch` 超过 1 的情况下, 相当于处理流程存在"并行";

### `yield flame\rabbitmq\produce(string $url[, array $options = array()]) -> flame\rabbitmq\producer`
连接 `RabbitMQ` 服务器, 打开通道, 注备生产消息；可用选项如下:
* `prefetch` - `Integer` - `RabbitMQ` 预读取数量, 0 < `prefetch` < 65536, 默认 `1`;
* `mandatory` - `Boolean` - 默认 `false`
* `immedate` - `Boolean` - 默认 `false`

**注意**:
* 设置 `prefetch` 超过 1 的情况下, 相当于处理流程存在"并行";


### `class flame\rabbitmq\consumer`
消费者

#### `yield consumer::run(callable $cb) -> void`
启动消费过程并持续消费, 调用 `$cb` 消费回调协程; 消费协程函数圆形如下:
``` PHP
void callback(flame\rabbitmq\message $message) {}
```

**注意**:
* 运行消费会阻塞当前协程, 可使用 `close()` 停止消费 (恢复当前协程);

#### `boolean consumer::confirm(flame\rabbitmq\message $message)`
确认消息已处理;

**注意**:
* 消费时指定 `"noack" => true` 选项, 无需确认或拒绝消息;

#### `boolean consumer::reject(flame\rabbitmq\message $message[, $requeue = false])`
丢弃或拒绝处理消息, 当 `$requeue` 设置为 `true` 时将消息放回原消息队列;

**注意**:
* 消费时指定 `"noack" => true` 选项, 无需确认或拒绝消息;

#### `yield consumer::close() -> void`
停止消费者运行过程 (恢复被 `yield run()` 暂停的协程), 不再调用消息回调;


### `class flame\rabbitmq\producer`
生产者

#### `void producer::publish(string $body[, string $routing_key)`
生产一条以 `$body` 为内容的消息, 可选的指定 `$routing_key`;

#### `void producer::publish(flame\rabbitmq\message $message[, string $routing_key])`
生产一条以 `$message` 为内容的消息, 可选的指定 `$routing_key` (默认为 `$message->routing_key` 属性指定的值);

**注意**：
* 此函数一般用于为特殊消息（需要指定属性或设置头信息）的生产；

### `class flame\db\rabbitmq\message`
消费生产过程中使用的消息对象，用于包裹实际的消息内容和其相关属性头信息等数据;

#### `message::__construct([string $body, [string $routing_key]])`
构建一个 `RabbitMQ` 消息, 可选的填充 `message::$body` 内容及设定 `message::$routing_key`;

#### `String message::$routing_key`
消息生产时指定的 ROUTING KEY；

#### `String message::$body`
消息体，内容；

#### `String message::$expiration`
#### `String message::$reply_to`
#### `String message::$correlation_id`
#### `String message::$priority`
#### `Integer message::$delivery_mode`
#### `Array message::$header`
头信息
#### `String message::$content_encoding`
#### `String message::$content_type`
#### `String message::$cluster_id`
#### `String message::$app_id`
#### `String message::$user_id`
#### `String message::$type_name`
#### `Integer message::$timestamp`
#### `String message::$message_id`
(用户可自行设定) 秒级时间戳;

**注意**:
* 上述属性(除 `$routing_key`/`$body` 外)均由用户自行指定，严格说，数据的含义、功能，在一定程度上也是由用户自行规划指定的；


#### `string message::__toString()`
方便消息处理, 返回消息体，与 `$body` 一致；

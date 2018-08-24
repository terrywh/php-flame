## `namespace flame\rabbitmq`

<!-- TOC depthFrom:3 -->

- [`yield flame\rabbitmq\connect(string $url[, array $options = array()]) -> flame\rabbitmq\client`](#yield-flame\rabbitmq\connectstring-url-array-options--array---flame\rabbitmq\client)
- [`class flame\rabbitmq\client`](#class-flame\rabbitmq\client)
    - [`flame\rabbitmq\consumer client::consume(string $queue_name[, array $options])`](#flame\rabbitmq\consumer-clientconsumestring-queue_name-array-options)
    - [`flame\rabbitmq\procuer producer::produce([string $exchange = "" [, array $options = array()]])`](#flame\rabbitmq\procuer-producerproducestring-exchange----array-options--array)
- [`class flame\rabbitmq\consumer`](#class-flame\rabbitmq\consumer)
    - [`yield consumer::run(callable $cb) -> void`](#yield-consumerruncallable-cb---void)
    - [`boolean consumer::confirm(flame\rabbitmq\message $message)`](#boolean-consumerconfirmflame\rabbitmq\message-message)
    - [`boolean consumer::reject(flame\rabbitmq\message $message[, $requeue = false])`](#boolean-consumerrejectflame\rabbitmq\message-message-requeue--false)
    - [`yield consumer::close() -> void`](#yield-consumerclose---void)
- [`class flame\rabbitmq\producer`](#class-flame\rabbitmq\producer)
    - [`void producer::publish(string $body[, string $routing_key)`](#void-producerpublishstring-body-string-routing_key)
    - [`void producer::publish(flame\rabbitmq\message $message[, string $routing_key])`](#void-producerpublishflame\rabbitmq\message-message-string-routing_key)
- [`class flame\db\rabbitmq\message`](#class-flame\db\rabbitmq\message)
    - [`message::__construct([string $body, [string $routing_key]])`](#message__constructstring-body-string-routing_key)
    - [`String message::$routing_key`](#string-messagerouting_key)
    - [`String message::$body`](#string-messagebody)
    - [`String message::$expiration`](#string-messageexpiration)
    - [`String message::$reply_to`](#string-messagereply_to)
    - [`String message::$correlation_id`](#string-messagecorrelation_id)
    - [`String message::$priority`](#string-messagepriority)
    - [`Integer message::$delivery_mode`](#integer-messagedelivery_mode)
    - [`Array message::$header`](#array-messageheader)
    - [`String message::$content_encoding`](#string-messagecontent_encoding)
    - [`String message::$content_type`](#string-messagecontent_type)
    - [`String message::$cluster_id`](#string-messagecluster_id)
    - [`String message::$app_id`](#string-messageapp_id)
    - [`String message::$user_id`](#string-messageuser_id)
    - [`String message::$type_name`](#string-messagetype_name)
    - [`Integer message::$timestamp`](#integer-messagetimestamp)
    - [`String message::$message_id`](#string-messagemessage_id)
    - [`string message::__toString()`](#string-message__tostring)

<!-- /TOC -->

提供 RabbitMQ 协程式客户端封装；**暂不支持**除生产消费以外的功能 (例如, declare queue/exchange bind queue 等), 请使用 RabbitMQ UI 界面做手工操作;

**示例**：
``` PHP
// ...
$client = yield flame\rabbitmq\connect("amqp://user:pass@127.0.0.1:5672/vhost");
$options = ["immediate" => true];
// 生产
$producer = $client->produce("exchange", ["immediate"=>true]);
// 简单消息生产
$producer->publish("this is a message payload", "this is the routing key");
// 属性消息生产
$message = new flame\rabbitmq\message("this is another message payload", "this is the routing key");
$message->user_id = "aaaaaa";
$message->header["aaaaaaa"] = "bbbbbbbb";
$producer->publish($message);
// 消费
$consumer = $client->consume("queue_name_1", ["nolocal" => true]);
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

### `yield flame\rabbitmq\connect(string $url[, array $options = array()]) -> flame\rabbitmq\client`
连接 `RabbitMQ` 服务器并打开通道, 返回客户端对象实例; 可用选项如下:
* `prefetch` - `Integer` - `RabbitMQ` 预读取数量, 0 < `prefetch` < 65536, 默认 `1`;

**注意**:
* 设置 `prefetch` 超过 1 的情况下, 相当于处理流程存在"并行";

### `class flame\rabbitmq\client`
客户端对象, 包括 生产, 消费在内的主体 API 都有 `client` 对象提供;

#### `flame\rabbitmq\consumer client::consume(string $queue_name[, array $options])`
创建一个将要消费指定队列 `$queue_name` 消费者, 使用独立的协程将每条带处理的消息回调 `$cb`; 可用选项如下:
* `nolocal` - `Boolean` - 默认 `false`
* `noack` - `Boolean` - 默认 `false`, 消息需确认消费成功, 否则无需确认;
* `exclusive` - `Boolean` - 默认 `false`

#### `flame\rabbitmq\procuer producer::produce([string $exchange = "" [, array $options = array()]])`
创建一个将要通过 `$exchange` 进行消息生产的生产者；可用选项如下:
* `mandatory` - `Boolean` - 默认 `false`
* `immedate` - `Boolean` - 默认 `false`

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
生产一条以 `$message` 为内容的消息 `$message` (可以补充指定其他属性及头信息), 可选的指定 `$routing_key`;

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
(用户可自行设定) 秒级时间戳;

#### `String message::$message_id`

#### `string message::__toString()`
方便消息处理, 返回消息体，与 `$body` 一致；

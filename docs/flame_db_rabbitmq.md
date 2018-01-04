### `namespace flame\db\rabbitmq`
提供 RabbitMQ 协程式客户端封装；基于 [rabbitmq-c](https://github.com/alanxz/rabbitmq-c) 封装。

**示例**：
``` PHP
<?php
// 生产
flame\go(function() {
	$producer = new flame\db\rabbitmq\producer("amqp://user:pass@127.0.0.1:5672/vhost", [ // 选项
		"mandatory" => false,
		"immediate" => false,
	], "amq.direct"); // 使用的 exchange 名称
	yield $producer->produce("this is a message", "this is the routing key", [ // 属性
		"type" => "aaaaaa",
		"headers" => [
			"a" => "bbbbbbbb",
		],
	]);
	yield $producer->produce("this is a message", "this is the routing key");
});
// 消费
flame\go(function() {
	$consumer = new flame\db\kafka\consumer("amqp://user:pass@127.0.0.1:5672/vhost", [ // 选项
		"no_ack" => true,
	], "test_queue"); // 或 ["test_queue1", "test_queue2"] 消费者可指定多个 队列 进行消费
	while(true) {
		$msg = yield $producer->consume();
		$msg = yield $producer->consume(5000); // 可选超时
		// ...
	}
});
```

#### `class flame\db\rabbitmq\producer`
生产者

##### `producer::__construct(string $url[, array $options][, string $exchange_name])`
连接由 `$url` 指定的 RabbitMQ 服务器并使用对应配置 `$options`、Exchange 名称 `$exchange_name` 构建生产者;

连接 `$url` 形式如下：
```
amqp://{USER}:{PASS}@{HOST}:{PORT}/{VHOST}
```

配置 `$options` 可选，可用配置项如下：
* `mandatory` - boolean -
* `immediate` - boolean - 

`$exchange_name` 可选，为空时使用内置的“无名称”的 `direct` 类型 Exchange；

##### `yield producer::produce(string $data[, string $routing_key[, array $properties]])`
生产一条消息，可选的指定消息的 ROUTING_KEY ，其规则与使用的的 EXCHANGE 的类型相关；

可用的属性如下：
* `content_type` - string
* `content_encoding` - string
* `headers` - array - ASSOC
* `delivery_mode` - integer
* `priority` - integer
* `correlation_id` - string
* `reply_to` - string
* `expiration` - string
* `timestamp` - integer - 时间戳，默认状态，框架自动填入当前“秒级时间戳”；
* `message_id` - string
* `type` - string
* `user_id` - string
* `app_id` - string
* `cluster_id` - string

**注意**：
* 一般来说，指定任何属性不会对消息的生产过程构成任何影响；
* `headers` 为关联数组，其 KEY 长度不能超过 128 字节，不支持潜逃数组；
* `timestamp` 可以手动填写，按 AMQP-0-9-1 标注，应使用秒级时间戳；框架会自动填写当前时间戳；

#### `class flame\db\kafka\consumer`
消费者

##### `consumer::__construct(string $url[, array $options][, string|array $queue_name])`
连接 `$url` 指定的 RabbmitMQ 服务器，以指定配置 `$options` 创建消费者，订阅消费 `$queue_name` 指定的队列；

连接 `$url` 形式如下：
```
amqp://{USER}:{PASS}@{HOST}:{PORT}/{VHOST}
```

配置 `$options` 可用配置项如下：
* `no_local` - boolean
* `no_ack`   - boolean
* `exclusive` - boolean
* `arguments` - array - ASSOC

**注意**：
* 以数组形式指定 `queue_name` 时可以订阅消费多个队列，但实际数据并无法区分其来自那个队列，如需要，可在生产时添加属性或头部信息；


##### `yield consumer::consume([integer $timeout = 0])`
消费（等待 RabbitMQ 返回）一条消息；可选的指定 `$timeout` 超时（单位 `ms` 毫秒），`0` 不超时；当指定超时设置时，若在指定时间内未能获取消息（队列空），则返回 `null`；

**示例**：
``` PHP
<?php
// $consumer = ...
$msg = yield $consumer->consume();
$msg = yield $consumer->consume(5000);
```

**注意**：
* 多个协程进行消费同一个 consumer 会导致未知错误；

##### `yield consumer::commit(object $msg)`
手动提交消息偏移；

**注意**：
* 需要手动提交，请设置 `enable.auto.commit` 为 `"false"`
* 相较之下，手动提交效率很低，请斟酌使用；

#### `class flame\db\kafka\message`
消费者消费过程返回的消息对象，用于包裹实际的消息内容和其相关数据

##### `string message::$key`
消息生产时指定的 ROUTING KEY；

##### `string message::$val`
消息体，内容；

##### `message::__toString()`
消息体，与 `$val` 一致；

##### `message::timestamp()`
时间戳；

**注意**：
* 这里会直接返回消息属性 `timestamp` ，由于该数值可以自行指定，故此处返回单位不严格为 
“秒”；

##### `message::timestamp_ms()`
时间戳；

**注意**：
* 这里会直接放回消息属性 `timestamp * 1000` 的值，由于该数值可以自行指定，故自出返回单位不严格为“毫秒”；

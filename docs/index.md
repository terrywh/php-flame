## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。目前，flame 中提供了如下功能：
1. [协程核心](/php-flame)；
	1. [核心协程函数](/php-flame)；
	2. [时间协程函数](/php-flame/flame_time) - 调度休眠、定时器；
	3. [操作系统函数](/php-flame/flame_os) - 异步进程、PHP 路径；
	4. [日志输出](/php-flame/flame_log)；
2. [协程式网络](/php-flame/flame_net) ；
	1. [辅助函数](/php-flame/flame_net)；
	2. [HTTP 客户端、处理器](/php-flame/flame_net_http)；
	3. [Unix Socket 客户端、服务端](/php-flame/flame_net)；
	4. [TCP 客户端、服务端](/php-flame/flame_net)；
	5. [UDP 客户端、服务端](/php-flame/flame_net)；
	6. [FastCGI 处理器](/php-flame/flame_net_fastcgi) - 挂接 Nginx 等实现 HTTP 服务；
3. [协程式数据库驱动](/php-flame/flame_db)：
	1. [Redis 客户端](/php-flame/flame_db) - 简单封装；
	2. [Mongodb 客户端](/php-flame/flame_db_mongodb) - 简单封装；
	3. [MySQL 客户端](/php-flame/flame_db_mysql) - 简单封装；
	4. [Kafka](/php-flame/flame_db_kafka) - 简单生产消费；
	4. [RabbitMQ](/php-flame/flame_db_rabbitmq) - 简单生产消费；

**源码**：
[https://github.com/terrywh/php-flame/](https://github.com/terrywh/php-flame/)

**示例**：
[https://github.com/terrywh/php-flame/tree/master/test/flame](https://github.com/terrywh/php-flame/tree/master/test/flame)

**使用**：
* 调用异步函数（Flame 提供）需要使用 yield 关键字；
* 调用异步函数的函数也是异步函数；
* 入口异步函数需要使用 flame\go() 包裹调用；
* 构造、析构函数等特殊函数无法定义为异步函数（不能调用异步函数或使用 yield 关键字）；

**示例**：
``` PHP
<?php
flame\init("fastcgi-server");
flame\go(function() {
	// 创建 http 处理器
	$handler = new flame\net\http\handler();
	// 设置默认处理程序
	$handler->handle(function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req);
		yield $res->end("hello world!");
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定处理程序
	$server->bind("127.0.0.1", 19001);
	yield $server->run();
});
flame\run();
```

完整示例可参考 [test/flame/net](https://github.com/terrywh/php-flame/tree/master/test/flame/net) 下相关代码；

## `namespace flame`

最基本的“协程”函数封装，例如生成“协程”，“协程”调度等；

#### `flame\init(string $app_name, array $options)`
可选，设置并初始化框架；设置 `$app_name` 应用名称将被用于重置进程名称；子进程启动时将获得额外的环境变量 `FLAME_CLUSTER_WORKER` 其值为进程编号（从 1 开始）；

**示例**：
``` php
<?php
flame\init("test_app", [ // 可选
	"worker" => 4, // 进程数，默认为 0 即不启动工作进程
]);
```

**注意**：
* 注意进程数不包含当前“主进程”，即指定 4 将启动额外的 4 个进程和当前 1 个主进程；
* 启动的“工作进程”与当前“主进程”完全平级功能相同（主进程额外包含维护、启动子进程功能）；
* 应用名称设置的进程名，后附加 `(fm)`（主进程）或 `(fw)` (子进程) 以示区分；
* 初始化动作必须在 `flame\run()` 前调用；

#### `flame\go(callable $g)`
生成并启动一个“协程”；协程即调度执行的线程内的独立“执行单位”，可以认为互相独立运行；

**示例**：
``` php
<?php
function g1() {
	yield 1;
	yield 2;
}
// 传入 Generator 函数的定义：
flame\go(g1);
flame\go(function() {
	yield 3;
	yield 4;

});
```

**注意**：
* 协程函数不会立即执行（将会在 `run()` 中调度启动）；

#### `flame\run()`
框架入口，所有协程调度在此处开始执行，直到所有“协程”结束；

**示例**：
``` PHP
<?php
flame\init("name");
flame\go(function() {
	// yield ...
});
flame\run(); // 实际程序在此循环调度
```

**注意**:
* 此函数调用一般放置在入口执行文件的最后，开始后本函数会阻塞进行协程运行、调度，直到所有协程执行结束；
* 由于所有异步调度动作在本函数中进行，故当异步函数流程发生错误时堆栈信息可能仅包含 `flame\run()` 运行过程；

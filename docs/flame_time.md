
## `namespace flame\time`

封装与时间有关的 API，例如获取当前时间（毫秒）、SLEEP 等；

#### `integer flame\time\now()`
获取当前时间戳（毫秒级）；

**注意**：
* 本函数返回的时间非系统实时的时钟，框架会在异步调度流程当中更新这个时钟；（使用阻塞函数等可能导致 now() 未及时更新而出现偏差）

#### `yield flame\time\sleep(integer $ms)`
当前协程休眠 `$ms` 毫秒；

#### `$tick = flame\time\after(integer $ms, callable $cb)`
在 `$ms` 毫秒后，执行 `$cb` 回调（仅执行一次）；返回 `class ticker` 的实例，请参考后续说明；

**注意**：
* 当 `$cb` 指定的回调函数为 `Generator Function` 时将启用单独的协程运行，否则仅作为普通回调；

#### `$tick = flame\time\tick(integer $ms, callable $cb)`
**每隔** `$ms` 毫秒后，执行 `$cb` 回调；返回 `class ticker` 的实例，请参考后续说明；

**示例**：
``` PHP
<?php
$tick = flame\time\tick(5000, function($tick) {
	$tick->stop(); // 终止计时器，即回调函数不会被再次调用了
	// 在这里调用 stop() 相当于使用 after(5000, function($tick) {})
});
```

**注意**：
* 当 `$cb` 指定的回调函数为 `Generator Function` 时将启用单独的协程运行；

### `class flame\time\ticker`
定时器

#### `ticker::__construct(integer $ms[, boolean $repeat = true])`
构造并指定定时器，指定时间间隔 `$ms` 毫秒，是否反复执行 `$repeat`

#### `ticker::start(callable $cb)`
指定回调函数，并启动定时器。

**示例**：
``` PHP
<?php
$count = 0;
$tick = new flame\time\ticker(2000, true);
$tick->start(function($tick) {
	global $count;
	// 例外 $tick 为同一个对象的不同引用
	if($count > 5) {
		$tick->stop(); // 停止定时器
	}
});
```

**注意**：
* 当 `$cb` 指定的回调函数为 `Generator Function` 时将启用单独的协程运行，否则仅作为普通回调；

#### `ticker::stop()`
停止定时器（若定时器回调函数还未执行，将不会被执行；已经执行的定时器将停止后续的回调）。

**注意**：
* 调用 `stop` 函数可以在回调函数内，也可以在其他协程；

## `namespace flame`

最基本的“协程”函数封装，例如生成“协程”，“协程”调度等；

#### `void flame\init(string $title[, array $options])`
设置并初始化框架, 进程标题 `$title` 及可能的配置；目前可用选项如下:

* `debug` `Boolean` - 默认 `true`, 在调试模式时:
	1. 工作进程崩溃不会自动重启(所有子进城退出后主进程结束);
	2.  
* `worker` `Integer` - 工作进程数量, 默认(至少) 1 个 (最多 256 个);
* `logger` `String` - 重定向日志输出到指定路径文件, 为空或默认状态会重定向到标准错误输出;

**示例**：
``` php
<?php
flame\init("test_app", [ // 可选
	"worker" => 4, // 进程数，框架会至少启动 1 个工作进程
]);
```

**注意**：
* 必须在所有框架函数执行前调用 `init` 进行初始化;
* 应用名称设置的进程名，后附加 `(flame/m)`（主进程）或 `(flame/${x})` (工作进程) 以示区分；
* !!! 一定在生产环境设置 `debug` 选项为 `false` !!!

#### `void flame\go(callable $g)`
生成并启动一个“协程”；

**示例**：
``` PHP
function g1() {
	yield 1;
	yield 2;
}
// 传入 Generator 函数的定义：
flame\go(g1);
// 回调
flame\go(function() {
	yield 3;
	yield 4;

});
class A {
	function b() {
		yield 4;
		yield 5;
	}
}
$a = new A();
// 对象成员
flame\go([$a, "b"]);
```

**注意**：
* 协程函数不会立即执行（将会在 `run()` 中调度启动）；

#### `void flame\run()`
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

### `namespace flame\log`
提供简单 log 日志 API，并提供简单的日志重载功能;

### `class flame\log\logger`
日志类封装

**示例**:

#### `logger::__construct([string $filepath])`
创建 `logger` 对象, 可选的将输出重定向到 `$filepath` 文件;

**注意**：
* 使用文件作为输出目标，在系统发生异常并导致退出时，该错误信息也会被记录进对应日志文件，并标记 `(PANIC)` 以区别其他类型；
* 使用文件作为输出目标，当进程或进程组接收到 `SIGUSR2` 信号时，将自动对日志输出文件进行 `ROTATE` 即关闭并重新按照路径打开；

#### `void logger::fail(mixed $message, ...)`
#### `void logger::warn(mixed $message, ...)`
#### `void logger::info(mixed $message, ...)`
向输出目标输出指定的日志内容，并补充时间戳和日志等级；数组将会被自动转换为 JSON 串，其他均使用 PHP 内置 `toString()` 转换为字符串输出。
#### `void logger::write(mixed $message, ...)`
向输出目标输出指定的日志内容，并补充时间戳（不含日志等级）；其余规则与上述 `fail`/`warn`/`info` 等函数一致；

**注意**：
* 每项函数参数输出前时均会前置一个空格用于分隔，若不需要请自行拼接输出内容并作为单个参数写入；

#### `void flame\log\fail(mixed $message, ...)`
#### `void flame\log\warn(mixed $message, ...)`
#### `void flame\log\info(mixed $message, ...)`
#### `void flame\log\write(mixed $message, ...)`
框架会自动生成一个全局默认的 `logger` 对象; 上述方法代理到该全局对象对应方法;

**注意**:
* 如果记录到文件全局 `logger` 配置为记录到文件, 当主进程收到 `SIGUSR2` 信号时, 自动进行日志重载 `rotate`;


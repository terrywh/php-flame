### `namespace flame\log`
提供简单 log 日志 API，并提供 “ROTATE” 和输出文件指定。这里的 "ROTATE" 指：当进程接收到 SIGUSR2 信号时，如果当前 logger 输出对象是 文件，关闭并重新打开目标路径的文件；

#### `flame\log\set_output(string $output)`
设置输出目标：
1. "stdout" - 将日志输出到“标准输出”；
2. "stdlog"/"stderr" - 将日志输出到“标准错误”；
3. 文件路径 - 将日志输出到指定文件；

**注意**：
* 使用文件作为输出目标，在系统发生异常并导致退出时，该错误信息也会被记录进对应日志文件，并标记 `(PANIC)` 以区别其他类型；
* 使用文件作为输出目标，当进程或进程组接收到 `SIGUSR2` 信号时，将自动对日志输出文件进行 `ROTATE` 即关闭并重新按照路径打开；

#### `yield flame\log\fail(mixed $message, ...)`
#### `yield flame\log\warn(mixed $message, ...)`
#### `yield flame\log\info(mixed $message, ...)`
向输出目标输出指定的日志内容，并补充时间戳和日志等级；数组将会被自动转换为 JSON 串，其他均使用 PHP 内置 `toString()` 转换为字符串输出。

**注意**：
* 每项函数参数输出前时均会前置一个空格用于分隔，若不需要请自行拼接输出内容并作为单个参数写入；

#### `yield flame\log\write(mixed $message, ...)`
向输出目标输出指定的日志内容，并补充时间戳（不含日志等级）；其余规则与上述 `fail`/`warn`/`info` 等函数一致；

**注意**：
* 每项函数参数输出前时均会前置一个空格用于分隔，若不需要请自行拼接输出内容并作为单个参数写入；

### `class flame\log\logger`
日志类封装，外层 `flame\log\*` 提供的 API 即对全局的默认 logger 实例进行操作。

#### `logger::set_output(string $output)`
#### `yield logger::fail(mixed $message, ...)`
#### `yield logger::warn(mixed $message, ...)`
#### `yield logger::info(mixed $message, ...)`
#### `yield logger::write(mixed $message, ...)`
与上述全局 API 功能一致，但仅对当前对象进行操作（而非全局日志对象）；

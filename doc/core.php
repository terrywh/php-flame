<?php
namespace flame;
/**
 * 运行（初始化框架, 设置服务、进程名称等相关配置，并启动主协程）
 * @param array  $options 选项配置:
 * 
 * | 选项 | 类型 | 功能 | 备注 |
 * | ---- | ---- | ---- | ---- |
 * | `service.name`    | string        | 服务名称，一般用于服务发现注册 | 可考虑使用域名或反向域名形式，例如 `user_center.xxx.com` / `com.xxx.user_center` 等 |
 * | `logger.target`   | array(string) | 日志输出目标，标识或文件 | 特定标识或文件路径：`["console","stdout","stderr","syslog","./x/y/z.log","../x/y/z.log","/x/y/z.log"]` |
 * | `logger.severity` | string        | 日志输出等级，低于该等级的目标将不被记录 | 日志等级：`"debug"` / `"info"` / `"notice"` / `"warning"` / `"error"` / `"critical"` / `"alert"` / `"emergency"` |
 * 
 * * 注意：
 * 1. 部分环境变量会对框架的运行产生影响：
 * 
 * | 环境变量名 | 功能 | 备注 |
 * | ---------- | ---- | ---- |
 * | `FLAME_MAX_WORKERS` | 工作进程数量 | 读写，默认为硬件核心数的 1/2 可设定 (0, 256] 范围 |
 * | `FLAME_CUR_WORKER`  | 工作进程编号 | 只读，由框架进行工作进程调度自行生成 |
 * 
 * 2. 本函数阻塞并持续运行直到所有协程结束；
 * 3. 用户须自行处理退出信号，并终止各协程，否则可能导致进程无法销毁；
 * 
 * @see flame\go()
 * @see flame\on()
 */
function run(array $options = [], callable $main) {}
/**
 * 启动协程，并执行指定函数
 * 
 * 注意：
 * 1. 框架使用 固定栈协程 (Fixed-sized Stack Coroutine)，包含 PHP 栈空间在内，单协程占用 ~128kB 内存；
 * 2. 所有启动的协程 在单一的一个线程中 内调度；
 * @param callable $cb 协程函数
 */
function go(callable $cb) {}
/**
 * 设置框架消息处理
 * @param string $event 事件名，目前可用的事件如下：
 * 
 * | 事件 | 触发时间 | 备注 | 
 * | ---- | -------- | ---- |
 * | quit | 进程收到退出信号 | 用户应通知协程结束，当所有协程退出后，进程结束 |
 *
 */
function on(string $event, callable $handler) {}

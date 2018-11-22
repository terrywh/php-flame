<?php
/**
 * 基本框架流程函数, 用于框架初始化, 启动协程等;
 */
namespace flame;
/**
 * 初始化框架, 设置进程名称及相关配置;
 * @param string $process_name 进程名称
 * @param array  $options 选项配置, 目前可用如下:
 *  * `worker` - 工作进程仓库
 *  * `logger` - 日志输出重定向目标文件(完整路径);
 */
function init($process_name, $options = []) {}
/**
 *  启动协程(运行对应回调函数)
 */
function go(callable $cb) {}
/**
 * 框架调度, 上述协程会在框架开始调度运行后启动
 */
function run() {}

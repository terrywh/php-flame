<?php
/**
 * 基本框架流程函数, 用于框架初始化, 启动协程等;
 * 框架可在两种模式下工作：
 * 
 * 1. 单进程模式：
 *  * 信号 SIGQUIT 立即终止进程；
 *  * 信号 SIGINT / SIGTERM 将 “通知” 进程退出 (触发 `quit` 回调)，等待超时后立即终止；
 *  * 信号 SIGUSR1 将切换 HTTP 服务器长短连状态 `Connection: close`；
 *  * 信号 SIGUSR2 日志文件将会被重新打开(或生成);
 * 
 * 2. 父子进程模式：
 *  * 使用环境变量 FLAME_MAX_WORKERS=X 启动父子进程模式；
 *  * 子进程存在环境变量 FLAME_CUR_WORKER=N 标识该进程标号，1 ~ FLAME_MAX_WORKERS
 *  * 主进程将自动进行日志文件写入，子进程自动拉起；
 *  * 端口监听使用 SO_REUSEPORT 在多个工作进程间共享，但不排除外部监听的可能；
 * 
 *  * 向主进程发送 SIGINT / SIGQUIT 将立即终止进程；
 *  * 项主进程发送 SIGTERM 将 ”通知“ 子进程退出 (触发 `quit` 回调)，并等待超时后立即终止；
 *  * 向主进程发送 SIGUSR2 信号该文件将会被重新打开(或生成);
 *  * 子进程继承主进程的 环境变量 / -c /path/to/php.ini 配置（通过命令行 -d XXX=XXX 的临时设置无效）；
 *  * 向主进程发送 SIGUSR1 信号将切换 HTTP 服务器长短连状态 `Connection: close`；
 *  * 向主进程发送 SIGRTMIN+1 将进行进程重载（陆续平滑停止当前工作进程并重新启动）
 */
namespace flame;
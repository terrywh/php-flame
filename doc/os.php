<?php
/**
 * 提供操作系统相关信息读取、进程操作等功能封装
 */
namespace flame\os;
/**
 * 信号
 */
const SIGTERM = 15;
/**
 * 信号
 */
const SIGKILL = 9;
/**
 * 信号
 */
const SIGINT  = 2;
/**
 * 信号
 */
const SIGUSR1 = 10;
/**
 * 信号
 */
const SIGUSR2 = 12;

/**
 * 获取当前系统网卡绑定地址等信息
 * @example
 *  array(2) {
 *      ["lo"]=>
 *      array(1) {
 *          [0]=>
 *          array(2) {
 *          ["family"]=>
 *          string(4) "IPv4"
 *          ["address"]=>
 *          string(9) "127.0.0.1"
 *          }
 *      }
 *      ["eth0"]=>
 *      array(1) {
 *          [0]=>
 *          array(2) {
 *          ["family"]=>
 *          string(4) "IPv4"
 *          ["address"]=>
 *          string(13) "10.110.16.197"
 *          }
 *      }
 *      }
 */
function interfaces():array {}
/**
 * 异步启动进程
 * @return 进程对象
 */
function spawn(string $command, array $argv = [], array $options = []):process {}
/**
 * 调用上述 spawn() 异步启动进程, 并等待其结束, 返回进程标准输出
 * @param array $options 目前可用的选项如下：
 *  * "cwd" - string - 工作路径;
 *  * "env" - array - 环境变量，K/V 结构文本;
 * @return string 进程标准输出内容
 */
function exec(string $command, array $argv, array $options = []):string {}

/**
 * 进程对象
 */
class process {
    /**
     * 向进程发送指定信号
     */
    function kill(int $signal = SIGTERM) {}
    /**
     * 等待进程结束
     */
    function wait() {}
    /**
     * 获取进程标准输出（若进程还未结束需要等待）
     */
    function stdout():string {}
    /**
     * 获取进程错误输出（若进程还未结束需要等待）
     */
    function stderr():string {}
}
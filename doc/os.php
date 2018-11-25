<?php

namespace flame\os;

const SIGTERM = 15;
const SIGKILL = 9;
const SIGINT  = 2;
const SIGUSR1 = 10;
const SIGUSR2 = 12;

/**
 * 获取当前系统网卡绑定地址
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
 * @return string 进程标准输出内容
 */
function exec(string $command, array $argv, array $options = []):string {}

/**
 * 进程对象
 */
class process {
    function kill(integer $signal = SIGTERM) {}
    /**
     * 等待进程结束
     */
    function wait() {}
    function stdout():string {}
    function stderr():string {}
}
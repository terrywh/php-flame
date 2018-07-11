### `namespace flame\reds`
提供基本的 `Redis` 客户端封装;

#### `yield flame\redis\connect(string $url) -> flame\redis\client`
连接由 `$url` 指定的 `redis` 服务器, 并返回客户端对象; `$url` 格式如下:

``` 
# 连接 认证 选择
redis://auth:${pass}@${host}:${port}/${db}
# 连接 认证
redis://auth:${pass}@${host}:${port}
# 连接
redis://${host}:${port}
```

### `class flame\redis\client`
封装 REDIS 协议的请求、命令执行，例如：

**示例**：
``` PHP
<?php
// ...
yield $cli->set("key","val");
$val = yield $cli->mget("key1", "key2");
$val = yield $cli->incrby("key3", 2);
// ...
```

**注意**:
* 实际功能函数基于 `__call()` 魔术函数实现大部分 Reids 指令，函数名称与指令名相同 (忽略大小写);
* 部分返回会按照 PHP 使用习惯处理为关联数组, 例如 `HGETALL` / `ZSCAN` 等等;
* 除特殊说明外所有指令都是"异步函数", 故需要使用 `yield` 关键字调用;


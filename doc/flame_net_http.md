### `namespace flame\net`
提供基本的 HTTP 协议网络协程式客户端、服务器封装；

### `class flame\net\http\client_request`

封装 HTTP 协议的客户端请求请求

#### `client_request::__construct(string $url[, mixed $data[, integer $timeout]])`
初始化并生成客户端请求对象：

* `$url` ：请求的地址；
* `$data`：可选，请求体，若存在，将自动设置当前请求 `$req->method='POST'`，可以为 `null`；
* `$timeout`：可选，请求超时，单位 `s`（秒），默认为 5s；

**示例**：
``` PHP
<?php
$req = new flame\net\http\client_request("http://www.baidu.com", null, 5);
```

#### `string client_request::$method`
HTTP 的方法，目前支持的有 GET , POST , PUT；

#### `string client_request::$url`
HTTP 的请求地址，不能为空；

#### `integer client_request::$timeout`
请求超时，单位 `ms` (毫秒)；

#### `array client_request::$header`
请求头部，可以用于定制请求头部；

**示例**：
``` PHP
<?php
// ...
$req->header["content-type"] = "application/x-www-form-urlencoded";
```

#### `mixed client_request::$body`
Array / String 当 $method = `PUT` 或 `POST` 时的请求体；
**示例**：
``` PHP
<?php
// ...
$req->body = "aaaaaaaaaa";
// 支持直接使用 Array 类型，自动序列化为 application/x-www-form-urlencoded 格式
$req->body = ["a"=> "b", "c"=> "d"];
```

### `class flame\net\http\client`
封装 libcurl 相关的 HTTP 处理功能

* 提供对 单个域名 的出口连接限制等功能；
* 复用 client 对象以约束资源占用；

#### `yield http_client::exec(client_request $request)`
执行指定请求并返回响应对象；

**示例**：
``` PHP
<?php
$cli = new flame\net\http\client();
$req1 = new flame\net\http\client_request("http://www.panda.tv");
$req2 = new flame\net\http\client_request("http://www.baidu.tv");
$res1 = yield $cli->exec($req1);
var_dump($res1);
$res2 = yield $cli->exec($req2);
var_dump($res2);
```

#### `yield flame\net\http\get(string $url, integer $timeout = 2500)`
简单的 `GET` 请求方法。

#### `yield flame\net\http\post(string $url, mixed $post, integer $timeout = 2500)`
简单的 `POST` 请求方法；
* 当 `$post` 为数组时自动拼装为 `K1=V1&K2=V2` 形式进行发送；
* 当 `$post` 为文本时直接进行发送；

**示例**：
``` PHP
<?php
$ret = yield flame\net\http\post("http://www.panda.tv", ["arg1"=>"val1","arg2"=>"val2"]);
var_dump($ret);
```

#### `yield flame\net\http\put(string $url, mixed $put, integer $timeout = 2500)`
简单的 `PUT` 请求方法。
* 当 `$put` 为数组时自动拼装为 `K1=V1&K2=V2` 形式进行发送；
* 当 `$put` 为文本时直接进行发送；

#### `yield flame\net\http\remove(string $url, integer $timeout = 2500)`
简单的 `DELETE` 请求方法。

### `class flame\net\http\server_request`
作为服务端时，收到的来自客户端、浏览器请求的描述对象；

#### `array server_request::$method`
请求方法，如 'GET' 'POST' 等；

#### `string server_request::$uri`
请求路径，**不含** 查询字符串 `query string`；

#### `array server_request::$query`
请求 `GET` 参数；数据来源于请求 `URL` 的 `PATH` 之后 `?` 与 `#` 之间 或 `URL` 结束之前的文本，并通过 `parse_str()` 进行解析得到；

#### `array server_request::$header`
请求头信息，注意：
* 所有请求头的字段名称均被处理为**小写**，字段值保持不变；
* 所有请求头的字段名称中包含的 `-` 将被替换为 `_`；
* 仅存在字段名而无字段值的请求头将被忽略（不存在于 `$header` 属性中）;

#### `array server_request::$cookie`
请求附带的 cookie 信息（已解析为数组）；

* 原始的 cookie 字符串可以使用 `$request->header["cookie"]` 获得；

#### `mixed server_request::$body`
请求体；

* 当请求类型 `Content-Type` 标记为 `application/x-www-form-urlencoded` 时，会通过 `parse_str()` 解析得到 K/V 数组 `array`；
* 当请求类型标记为 `multipart/form-data` 时，会通过内置解析器得到 K/V 数组 `array` ；
* 其他请求类型仅能得到原始数据 `string`；

**注意**：
* 文件上传请求的请求体会按照 `multipart/form-data` 进行解析，**没有** 生成 `PHP` 中类似 `$_FILES` 的结构；

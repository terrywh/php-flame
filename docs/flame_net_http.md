### `namespace flame\net`
提供基本的 HTTP 协议网络协程式客户端封装；底层使用 curl 并内置使用 nghttp2 提供了 HTTP/2 支持；

### `class flame\net\http\client_request`

封装 HTTP 协议的客户端请求请求

#### `client_request::__construct(string $url[, mixed $data[, integer $timeout]])`
初始化并生成客户端请求对象：

* `$url` ：请求的地址；
* `$data`：可选，请求体，若存在，将自动设置当前请求 `$req->method='POST'`，可以为 `null`；
* `$timeout`：可选，请求超时，单位 `ms`（毫秒），默认为 2500ms；

**示例**：
``` PHP
<?php
$req = new flame\net\http\client_request("http://www.baidu.com", null, 5);
```

#### `client_request::ssl(array $options)`
设置 ssl 相关选项，可选的支持如下选项：
**示例**：
``` PHP
<?php
// $req  = ....
$request->ssl([
	"verify" => "none", // 可选，peer - 验证对端证书真实性 / host - 验证服务器 / both - 前两者均验证 / none - 前两者均不验证
	"cert"   => "/data/htdocs/flame.terrywh.net/etc/cert.pem", // 可选，证书，类型自动按照后缀名设定 ( pem / der )
	"key"    => "/data/htdocs/flame.terrywh.net/etc/key.pem",  // 可选，私钥，类型自动按照后缀名设定 ( pem / der / eng )
	"pass"   => "123456", // 可选，私钥密钥
]);
```

#### `string client_request::$method`
HTTP 的方法，目前支持的有 GET , POST , PUT；

#### `string client_request::$url`
HTTP 的请求地址，不能为空；

#### `integer client_request::$timeout`
请求超时，单位 `ms` (毫秒)；

#### `array client_request::$header`
请求头部，可以用于定制请求头部（KEY/VALUE 关联数组）；

**示例**：
``` PHP
<?php
// ...
// 单一设置
$req->header["content-type"] = "application/x-www-form-urlencoded";
// 统一设置
$req->header = ["content-type"=>"application/x-www-form-urlencoded"];
```

**注意**：
* 设置 Cookie 时请示用专用的 $cookie 属性数组，否则可能导致相互覆盖；

#### `array client_request::$cookie`
请求头部定制 Cookie 项；

**注意**：
* 设置 Cookie 不要使用 $header ，否则可能导致相互覆盖；

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
封装 libcurl 相关的 HTTP 处理功能，如无特殊需求，请尽量使用同一个 `client` 发送若干个请求（而非创建多个 `client` 对象），以减少资源占用；

#### `client::__construct(array $options)`
创建请求客户端对象，支持的属性如下示例：

**示例**：
``` PHP
<?php
$cli = new flame\net\http\client([
	"conn_share" => "pipe", // null/"" => 不进行排队或并行
	                 // "pipe" => 管道传输 HTTP/1.1 请求（同一连接）
	                 // "plex" => 并行传输 HTTP/2 请求（STREAM）
					 // "both"  => PIPE + PLEX
	"conn_per_host" => 4,  // 默认 2，同 HOST:PORT 请求建立的最大连接数
	"pipe_per_conn" => 2,  // 默认 4，单连接 HTTP/1.1 管道排队数量
]);

#### `yield client::exec(client_request $request)`
执行指定请求并返回响应对象 `flame\net\http\client_response`；

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

### `class flame\net\http\client_response`
客户端请求的响应，可自动转换为文本 (`toString()`)，也可以按属性访问对应数据；

#### `integer client_response::$status`
响应码，例如 `200`；

#### `array client_response::$header`
HTTP 响应头；

#### `array client_response::$cookie`
COOKIE，对应 HTTP 响应头中 Set-Cookie 的部分；

#### `string client_response::$body`
HTTP 响应体；

#### `string client_response::toString()`
HTTP 响应体，方便直接将对象作为文本使用；

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
* 所有请求头的字段名中包含 “\_” （下划线），将会被替换为 “-” (减号)；
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

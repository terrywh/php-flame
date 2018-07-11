### `namespace flame\http`
提供基本的 HTTP/1 协议服务器, 客户端封装；

### `class flame\http\client_request`
封装 HTTP 协议的客户端请求请求

#### `client_request::__construct(string $url[, mixed $data[, integer $timeout]])`
初始化并生成客户端请求对象：

* `$url` ：请求的地址；
* `$data`：可选，请求体，若存在，将自动设置当前请求 `$req->method='POST'`，可以为 `null`；
* `$timeout`：可选，请求超时，单位 `ms`（毫秒），默认为 3000ms；

**示例**：
``` PHP
<?php
$req = new flame\http\client_request("http://www.baidu.com", null, 5);
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
* 未设置 `Content-Type` 时, 将默认设置为 "application/x-www-form-urlencoded" 类型;
* 将 `Content-Type` 设置为 "application/x-www-form-urlencoded" 时, 数组 `$body` 会通过 `http_build_query` 转换为文本;
* 将 `Content-Type` 设置为 "application/json" 时, 数组 `$body` 会通过 `json_encode` 转换为文本;

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
// 支持直接使用 Array 类型，参考 $header 相关说明;
$req->body = ["a"=> "b", "c"=> "d"];
```

### `class flame\http\client_response`
客户端请求的响应，可自动转换为文本 (`toString()`)，也可以按属性访问对应数据；

#### `integer client_response::$status`
响应码，例如 `200`；

#### `array client_response::$header`
HTTP 响应头，K/V 数组；

**注意**：
* 所有 KEY 统一被处理为小写；

#### `array client_response::$cookie`
COOKIE，对应 HTTP 响应头中 Set-Cookie 的部分；每项元素的 KEY 为该项 Cookie 的名称，VAL 为该项 Cookie 的所有属性（例如 expire / path 等）对应的 K/V 关联数组；

#### `string client_response::$rawBody`
HTTP 响应体;

#### `mixed client_response::$body`
HTTP 响应体 (`Content-Type: application/json` 时, 自动解析为 `Array` 数组);

#### `string client_response::__toString()`
HTTP 响应体，方便直接将对象作为文本使用；

### `class flame\http\client`
HTTP 客户端, 用于执行 请求; 同一个 `client` 可重复使用, 发送若干个 `client_request` 请求 (控制连接数量)；

#### `client::__construct(array $options)`
创建请求客户端对象，支持的属性如下示例：

* `connection_per_host` `integer` - // 指定域名:端口最大连接数量, 默认 2 ( <= 512 )

**示例**：
``` PHP
<?php
$cli = new flame\http\client([
	"connection_per_host" => 4,  
]);
```

#### `yield client::exec(client_request $request) -> flame\http\client_response`
执行指定请求并返回响应对象；

**示例**：
``` PHP
<?php
$cli = new flame\http\client();
$req1 = new flame\http\client_request("http://www.google.com");
$req2 = new flame\http\client_request("http://www.baidu.tv");
$res1 = yield $cli->exec($req1);
var_dump($res1);
$res2 = yield $cli->exec($req2);
var_dump($res2);
```

**注意**：
* 执行请求失败可能抛出异常（例如：超时，无法连接等）；

#### `yield client::get(string $url[, integer $timeout = 3000]) -> flame\http\client_response`
简单的 `GET` 请求方法（自动生成 `client_request` 对象并调用上述 exec 方法）；

#### `yield client::post(string $url, mixed $data[, integer $timeout = 3000])`
简单的 `POST` 请求方法（自动生成 `client_request` 对象并调用上述 exec 方法）；
* 当 `$post` 为数组时自动拼装为 `K1=V1&K2=V2` 形式进行发送；
* 当 `$post` 为文本时直接进行发送；

**示例**：
``` PHP
<?php
$ret = yield flame\http\post("http://www.baidu.tv", ["arg1"=>"val1","arg2"=>"val2"]);
var_dump($ret);
```

#### `yield client::put(string $url, mixed $data[, integer $timeout = 3000]) -> flame\http\client_response`
简单的 `PUT` 请求方法, 与 `post` 类似;

#### `yield client::delete(string $url[, integer $timeout = 3000]) -> flame\http\client_response`
简单的 `DELETE` 请求方法, 与 `get` 类似；

#### `yield flame\http\get(string $url[, integer $timeout = 3000]) -> flame\http\client_response`
#### `yield flame\http\post(string $url, mixed $post[, integer $timeout = 3000]) -> flame\http\client_response`
#### `yield flame\http\put(string $url, mixed $put[, integer $timeout = 3000]) -> flame\http\client_response`
#### `yield flame\http\delete(string $url, integer $timeout = 3000) -> flame\http\client_response`
#### `yield flame\http\exec(client_request $request) -> flame\http\client_response`
框架会在初始化时建立一个默认全局的 `client`, 上述函数代理到该全局对象的对应方法;

### `class flame\http\server`
HTTP/1 服务器；

**示例**:
``` PHP
flame\go(function() {
	$server = new flame\http\server("127.0.0.1:7678");
	$server->before(function($req, $res, &$next) {
		if(...) {
			$next = false;
		}else{
			$req->data["user"] = ...;
			$req->data["time"] = flame\time\now();
		}
	})->get("/hello", function($req, $res) {
		// 返回 content-length: 6 形式
		$res->body = "abcdef";
	})->post("/hello", function($req, $res) {
		// 返回 transfer-encoding: chunked 形式
		yield $res->write("abc");
		yield $res->end("def");
	})->after(function($req, $res, $next) {
		flame\log\info("request elapse:", flame\time\now() - $req->data["time"], "ms");
	});
})
```

#### `flame\http\server server::get(string $path, callable $cb)`
#### `flame\http\server server::post(string $path, callable $cb)`
#### `flame\http\server server::put(string $path, callable $cb)`
#### `flame\http\server server::delete(string $path, callable $cb)`
分别用于设置 GET / POST / PUT / DELETE 请求方法对应路径的处理回调协程, 并返当前服务器对象 (用于串联调用)；`$cb` 原型如下：

``` PHP
void callback(flame\http\server_request $req, flame\http\server_response $res);
// $req - 请求对象
// $res - 响应对象
```

#### `server server::before(callable $cb)`
设置一个在每次请求处理过程开始前被执行的回调协程, 并返当前服务器对象 (用于串联调用); `$cb` 原型如下：

``` PHP
void callback(flame\http\server_request $req, flame\http\server_response $res);
// $req - 请求对象
// $res - 响应对象
```

**注意**:
* 可以在 `before` 对应回调协程中改变当前请求 `$method` 及 `$path` (重定向到指定的自定有的路径处理过程); 

#### `server server::after(callable $cb)`
设置一个在每次请求处理过程结束后被执行的回调协程, 并返当前服务器对象 (用于串联调用)；`$cb` 原型如下:

``` PHP
void callback(flame\http\server_request $req, flame\http\server_response $res);
// $req - 请求对象
// $res - 响应对象
```

#### `void server::close()`
关闭服务器, 停止处理连接, 并清理已设置的所有处理器;

**注意**:
* 关闭服务器后, 阻塞在 `yield run()` 的协程将恢复运行;

### `class flame\http\server_request`
作为服务端时，收到的来自客户端、浏览器的请求封装对象；

#### `array server_request::$method`
请求方法，如 'GET' 'POST' 等，统一处理为大写；

#### `string server_request::$path`
请求路径；

**注意**：
* 请求路径中 **不包含** 查询字符串 `query string`；

#### `array server_request::$query`
请求 `GET` 参数；数据来源于请求 `URL` 中 `PATH` 之后 `?` 与 `#` 之间 或 `URL` 结束之前的文本，并通过 `parse_str()` 进行解析得到 K/V 数组；

#### `array server_request::$header`
请求头信息，注意：
* 所有请求头的字段名称均被处理为**小写**，字段值保持不变；
* 所有请求头的字段名中包含 “\_” （下划线），将会被替换为 “-” (减号)；
* 仅存在字段名而无字段值的请求头将被忽略（不存在于 `$header` 属性中）;

#### `array server_request::$cookie`
请求附带的 `Cookie` 信息，K/V 数组；

**注意**：
* 原始的 `Cookie` 字符串可以使用 `$request->header["cookie"]` 获得；

#### `mixed server_request::$body`
请求体；存在如下可能：

* 当请求类型 `Content-Type` 标记为 `application/x-www-form-urlencoded` 时，会通过内置解析器得到 K/V 数组；
* 当请求类型 `Content-Type` 标记为 `multipart/form-data` 时，会通过内置解析器得到 K/V 数组 ；
* 当请求类型 `Content-Type` 标记为 `application/json` 时，会通过 PHP 引擎的 `json_decode` 解析得到 K/V 数组；
* 其他请求类型仅能得到原始数据 `string`；

**注意**：
* 文件上传请求的请求体类型为 `multipart/form-data` ，提取 `name` 作为 KEY 文件内容数据作为 VAL，**不会** 生成 `PHP` 中类似 `$_FILES` 的结构；

#### `server_request::$data`
默认为 null，可用于在 before / handle / after 之间传递数据；


### `class flame\http\server_response`
由上述 `server` 生成，并回调传递给处理函数使用，用于返回响应数据；

**注意**：
1. 使用 `$res->body = "aaaaaa";` 形式返回 `Content-Length: 6` 形式的响应;
2. 使用 `$res->write()/end()` 形式返回 `Transfer-Encoding: chunked` 形式的响应;


#### `array server_response::$header`
响应头部 KEY/VAL 数组，默认包含 `Content-Type: text/plain`（）；其他响应头请酌情考虑添加、覆盖；

**示例**：
``` PHP
<?php
$res->header["Content-Type"] = "text/html";
$res->header["X-Server"] = "Flame/0.7.0";
```

**注意**：
* 所有输出的 HEADER 数据 **区分大小写**；

#### `server_response::set_cookie(string $name [, string $value = "" [, int $expire = 0 [, string $path = "" [, string $domain = "" [, bool $secure = false [, bool $httponly = false ]]]]]])`

定义一个 Cookie 并以响应头 [Set-Cookie](https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Headers/Set-Cookie) 形式下发; 其中 `$expire` 参数存在两种形式:
* 当其值较小时 ( < 30 * 86400) 表示 Cookie 的有效时间长度 (以当前时间开始), 下发 Max-Age 及 Expire 字段 (兼容 IE < 9 浏览器);
* 当其值较大时 (>= 30 * 86400) 表示 Cookie 的销毁时间点 (一般设置到未来的时间点), 下发 Expire 字段; 

**注意**:
* 在 `writer_header` (Transfer-Encoding: Chunked) 或 处理协程结束前 (Content-Length: xx) 时才实际进行发送；
* 当设置 `$expire` 为过去时间点时, 一般浏览器会删除对应的 Cookie 项目;

#### `yield server_response::write_header(integer $status_code)`
设置并输出响应头部（含上述响应头 KEY/VAL 及响应状态行），请参照标准 HTTP/1.1 STATUS_CODE 设置数值；

#### `yield server_response::write(string $data)`
输出指定响应内容，若还未发送相应头，将自动调用 `write_header` 发送相应头（默认 `200 OK`）；请参考 `handle()` 的相关示例；

**注意**：
* 请谨慎在多个协程中使用 `write`/`end` 函数时，防止输出顺序混乱；

#### `yield server_response::end([string $data])`
结束请求，若还未发送响应头，将自动调用 `write_header` 发送相应头（默认 `200 OK`）；可选输出响应内容；输出完成后结束响应；

#### `yield server_response::file(string $root, string $path)`
将一个 `$root` 目录为根, 由 `$path` 指定路径的文件作为响应返回;

**注意**:
* 由于安全问题, `$path` 指定的路径将被自动处理到以 `$root` 为根目录的路径下 (不允许访问超出 `$root` 以外的文件);

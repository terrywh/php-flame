<?php
flame\init("http_1");

// 清理超时连接
// 清理被断开连接
/*flame\go(function() {*/
    //for($i = 0; $i < 100; ++$i) {
        //flame\http\get("http://127.0.0.1:8081/");
        //flame\time\sleep(1000);
    //}
//});


/*flame\go(function() {*/
    //$req = new flame\http\client_request("http://127.0.0.1:8080/", null, 5000);
    //$req->method = "GET";
    //$req->body = "aaaaaaaaaa";
    //$cli->exec($req);

    //flame\time\sleep(100000);
//});

// 多协程并发
/*$opt["connection_per_host"] = 20;*/
//$cli = new flame\http\client($opt);
//for($i = 0; $i < 300; $i++) {
    //flame\go(function() use($cli) {
        //$req = new flame\http\client_request("http://127.0.0.1:8080/", null, 5000);
        //$req->method = "GET";
        //$req->body = "aaaaaaaaaa";
        //$cli->exec($req);
    //});
/*}*/


// 直接使用全局连接测试
// core dump
flame\go(function() {
    for($i = 5000; $i < 5500; $i++) {
        flame\go(function() use($i) {
            //flame\time\sleep(rand(100,200));
            flame\http\get("http://127.0.0.1:8081/", 30000);
            var_dump("global client " . $i);
        });
    }
});

flame\go(function() {
    for($i = 1000; $i < 1500; $i++) {
        flame\go(function() use($i) {
            //flame\time\sleep(rand(100,200));
            flame\http\get("http://127.0.0.1:8081/", 30000);
            var_dump("global client " . $i);
        });
    }
});

// 使用独立连接测试
/*flame\go(function() {*/
    //$opt["connection_per_host"] = 1;
    //$cli = new flame\http\client($opt);
    //for($i = 0; $i < 100; $i++) {
        //flame\go(function() use($cli) {
            //$req = new flame\http\client_request("http://127.0.0.1:8080/", null, 5000);
            //$req->method = "GET";
            //$req->body = "aaaaaaaaaa";
            //$cli->exec($req);
        //});
        //flame\time\sleep(100);
    //}
    //echo "user client";
//});
//flame\go(function() {
    //$opt["connection_per_host"] = 1;
    //$cli = new flame\http\client($opt);
    //for($i = 0; $i < 100; $i++) {
        //flame\go(function() use($cli) {
            //$req = new flame\http\client_request("http://127.0.0.1:8080/", null, 5000);
            //$req->method = "GET";
            //$req->body = "aaaaaaaaaa";
            //$cli->exec($req);
        //});
        //flame\time\sleep(100);
    //}
    //echo "user client";
/*});*/

flame\run();

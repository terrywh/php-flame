<?php
flame\init("http_1");

flame\go(function() {
    flame\http\get("http://127.0.0.1:8081/", 3000);
    flame\http\post("http://127.0.0.1:8081/", "post", 3000);
    flame\http\put("http://127.0.0.1:8081/", "put", 3000);
    flame\http\delete("http://127.0.0.1:8081/", 3000);
});
// 使用独立连接测试
flame\go(function() {
    $opt["connection_per_host"] = 1;
    $cli = new flame\http\client($opt);
    for($i = 0; $i < 50; $i++) {
        flame\go(function() use($i, $cli) {
            //flame\time\sleep(rand(30, 50));
            for($j = 0; $j < 40; ++$j) {
                $req = new flame\http\client_request("http://127.0.0.1:8081/get", null, 500);
                $req->method = "GET";
                $req->body = "aaaaaaaaaa";
                $cli->exec($req);
            //var_dump("user client -------" . $i);
            }
            //var_dump('sleep ------1------');
        });
        //flame\time\sleep(rand(10, 20));
        //var_dump('sleep ------2------');
    }
    flame\time\sleep(3000);
});
flame\go(function() {
    $opt["connection_per_host"] = 1;
    $cli = new flame\http\client($opt);
    for($i = 0; $i < 40; $i++) {
        flame\go(function() use($i, $cli) {
            flame\time\sleep(rand(30, 50));
            for($j = 0; $j < 50; ++$j) {
                $req = new flame\http\client_request("http://127.0.0.1:8081/get", null, 500);
                $req->method = "GET";
                $req->body = "aaaaaaaaaa";
                $cli->exec($req);
            }
            //var_dump("user client +++++++" . $i); 
            //var_dump('sleep ------8------');
        });
        //flame\time\sleep(rand(10, 20));
        //flame\time\sleep(30);
        //var_dump('sleep ------9------');
    }
    //flame\time\sleep(2000);
});

// 清理超时连接
// 清理被断开连接
/*flame\go(function() {*/
    //for($i = 0; $i < 1; ++$i) {
        //flame\http\get("http://127.0.0.1:8081/");
        //flame\time\sleep(1000);
    //}
/*});*/


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
    for($i = 0; $i < 50; $i++) {
        flame\go(function() use($i) {
            for($j = 0; $j < 40; ++$j) {
                flame\time\sleep(rand(10,20));
                flame\http\get("http://127.0.0.1:8081/get", 300);
                var_dump("global client " . $i . " " . $j);
            }
        });
    }
    flame\time\sleep(3000);
});

flame\go(function() {
    for($i = 500; $i < 1500; $i++) {
        flame\go(function() use($i) {
            //flame\time\sleep(rand(100,200));
            flame\http\get("http://127.0.0.1:8081/get", 300);
            var_dump("global client " . $i);
        });
    }
});

flame\run();

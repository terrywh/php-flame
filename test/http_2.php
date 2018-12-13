<?php
flame\init("http_2");

flame\go(function() {
    $server = new flame\http\server(":::56120");
    $poll;

    $server->before(function($req, $res, $m) {
        $req->data["start"] = flame\time\now();
        if(!$m) {
            $res->status = 404;
            $res->body = "404 Not Found\n";
            return false; // 阻止后续 handle / after 的运行
        }
    })->POST("/", function($req, $res) {
        $res->status = 200;
        $res->header["Content-Type"] = "application/json";
        $res->body = $req->body;
        // 文件上传
        var_dump($req->file);
        // return $res->body = json_encode($req);
    })->get("/poll", function($req, $res) use(&$poll) {
        $res->header["Content-Type"] = "text/event-stream";
        $res->header["Cache-Control"] = "no-cache";
        $res->write_header(200);
        $poll = $res;
        // flame\go(function() use($res) {
        //     var_dump($res);
        //     for($i=0;$i<50;++$i) {
        //         flame\time\sleep(1000);
        //         echo "write\n";
        //         $res->write("event: time\n");
        //         $res->write("data: ".flame\time\now()."\n\n");
        //     }
        //     echo "done1\n";
        // });
    })->get("/404", function($req, $res) {
        $res->status = 404;
        $res->header["Connection"] = "close";
        $res->body = "404 Not Found\n";
    })->get("/push", function($req, $res) use(&$poll) {
        if($poll) {
            $poll->write("event: time\n");
            $poll->write("data: ".flame\time\now()."\n\n");
            $res->body = true;
        }else{
            $res->body = false;
        }
    })->get("/abc", function($req, $res) {
        $res->header["Content-Type"] = "text/html";
        $res->file(__DIR__, "/coroutine_1.php");
    })->get("/from", function($req, $res) {
        $r = flame\http\get("http://127.0.0.1:56120/");
        flame\time\sleep(10000);
        $res->body = $r->body;
    })->after(function($req, $res, $m) {
        $end = flame\time\now();
        // echo "elapsed: ", ($end - $req->data["start"]), "ms\n";
    });
    $server->run();
    echo "done2\n";
});

// flame\on("quit", function() use($server) {
//     echo "quit\n";
//     $server->close();
// });

flame\run();
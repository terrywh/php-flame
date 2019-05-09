<?php
flame\init("http_2");

class JsonObject implements JsonSerializable {
    function jsonSerialize()
    {
        $date = new DateTime();
        return ["date" => $date->unix()];
    }
}

flame\go(function() {
    $server = new flame\http\server(":::56101");
    $poll;

    $server->before(function($req, $res, $m) {
        $req->data["start"] = flame\time\now();
        if(!$m) {
            $res->status = intval(substr($req->path, 1));
            $res->header["Access-Control-Allow-Origin"] = "*";
            $res->header["Content-Type"] = "application/json";
            $res->body = ["a" => "bbbb"];
            return false;
        }
    })->get("/", function($req, $res) {
        $req->header["test1"] = "string";
        $req->header["test2"] = 123456;
        $res->set_cookie("a","b", 3600);
        $res->body = json_encode($req);
    })->post("/", function($req, $res) {
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
    })->get("/json", function($req, $res) {
        $res->header["content-type"] = "application/json";
        $res->body = ["data" => new JsonObject()];
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

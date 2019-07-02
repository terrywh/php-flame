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
    $q = new flame\queue();

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
    })->get("/poll", function($req, $res) use($q) {
        $res->header["Content-Type"] = "text/event-stream";
        $res->header["Cache-Control"] = "no-cache";
        $res->write_header(200);
        while($data = $q->pop()) {
            $res->write($data);
        }
    })->get("/push", function($req, $res) use($q) {
        $q->push("event: time\ndata: ".flame\time\now()."\n\n");
        $res->body = "done";
    })->get("/file", function($req, $res) {
        $res->header["Content-Type"] = "text/html";
        $res->file(__DIR__, "/coroutine_1.php");
    })->get("/exception", function($req, $res) {
        throw new exception("Some Exception");
        $res->body = "done";
    })->get("/json", function($req, $res) {
        $res->header["content-type"] = "application/json";
        $res->body = ["data" => new JsonObject()];
    })->after(function($req, $res, $m) {
        $end = flame\time\now();
        // echo "elapsed: ", ($end - $req->data["start"]), "ms\n";5
    });
    $server->run();
    echo "done2\n";
});

// flame\on("quit", function() use($server) {
//     echo "quit\n";
//     $server->close();
// });

flame\run();

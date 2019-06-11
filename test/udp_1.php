<?php
flame\init("udp_1");

flame\go(function() {
    $server = new flame\udp\server(":::6666");
    flame\go(function() use($server) {
        flame\time\sleep(1000);
        $socket = new flame\udp\socket();
        $socket->send_to("hello", "127.0.0.1:6666"); // 服务器 IPv4 兼容, 也可以收到
        $socket->send_to("world", "::1:6666");
        $socket->send_to("!", "localhost:6666");
        flame\time\sleep(1000);
        $socket = flame\udp\connect("127.0.0.1:6666");
        $socket->send("hello");
        $socket->send("world!");
        echo "received: ". $socket->recv()."\n";
        flame\time\sleep(10000);
        $server->close();
    });
    $server->run(function($data, $from) use($server) {
        echo "(".$from.") [".$data."]\n";
        $server->send_to($data, $from); // echo back
    });
});

flame\go(function() {
    $socket = flame\udp\connect("127.0.0.1:6666");
    for($i=0;$i<10;++$i) {
        $socket->send("hello");
        flame\time\sleep(100);
    }
});

flame\run();

<?php
flame\init("tcp_1");

flame\go(function() {
    $server = new flame\tcp\server("127.0.0.1:8687");
    flame\go(function() use($server) {
        flame\time\sleep(10000);
        echo "closing\n";
        $server->close();
    });
    $server->run(function($socket) {
        echo "connection accepted\n";
        while($line = $socket->read("\n")) {
            var_dump($line);
            $socket->write($line);
        }
        echo "disconnected\n";
    });
    echo "server closed\n";
});
flame\go(function() {
    flame\time\sleep(1000);
    $socket = flame\tcp\connect("127.0.0.1:8687");
    
    for($i=0;$i<10;++$i) {
        $socket->write("aaaaaa\nbbbbbb\n");
        $socket->read("\n");
        flame\time\sleep(100);
        $socket->read("\n");
        flame\time\sleep(100);
    }
});

flame\run();

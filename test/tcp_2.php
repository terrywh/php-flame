<?php
flame\init("tcp_2");

flame\go(function() {
    $server = new flame\tcp\server("0.0.0.0:8687");
    $server->run(function($socket) {
        $length = 0;
        while( ($line = $socket->read("\n")) !== null) {
            echo $line;
            if(substr($line, 0, 14) == "Content-Length") {
                $length = intval(substr($line, 16));
            }else if($line == "\r\n") {
                if($length > 0) {
                    $body = $socket->read($length);
                    echo $body, "\n";
                }
                flame\time\sleep(200);
                $socket->write("HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nok");
            }
        }
    });
    echo "done.\n";
});

flame\run();

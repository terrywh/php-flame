<?php
flame\init("rabbitmq_1");

flame\go(function() {
    $consumer = flame\rabbitmq\consume("amqp://username:password@host:port/vhost", "test_q1");
    flame\go(function() use($consumer) {
        flame\time\sleep(5000);
        echo "close\n";
        $consumer->close();
    });
    $consumer->run(function($msg) use($consumer) {
        var_dump($msg);
        $consumer->confirm($msg);
    });
    echo "consumer done\n";
});

flame\go(function() {
    $producer = flame\rabbitmq\produce("amqp://username:password@host:port/vhost");
    for($i=0;$i<20;++$i) {
        flame\time\sleep(20);
        echo $i * 2, "\n";
        $producer->publish("", "this is the data", "test_q1");
        flame\time\sleep(20);
        $message = new flame\rabbitmq\message("abcdefg", "test_q1");
        $message->header["a"] = 1234567890123;
        $message->header["b"] = "Asd";
        $message->header["c"] = 123.456;
        echo $i * 2 +1, "\n";
        $producer->publish("", $message);
    }
});

flame\run();

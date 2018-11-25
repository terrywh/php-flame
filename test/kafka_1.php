<?php
flame\init("kafka_1");

flame\go(function() {
    $consumer = flame\kafka\consume([
        "bootstrap.servers" => "host1:port1,host2:port2",
        "group.id" => "flame-test-consumer",
        "auto.offset.reset" => "smallest",
    ], ["test"]);
    flame\go(function() use($consumer) {
        // 60 秒后关闭消费者
        flame\time\sleep(60000);
        $consumer->close();
    });
    $consumer->run(function($msg) {
        var_dump($msg);
        flame\time\sleep(30);
    });
});


flame\go(function() {
    $producer = flame\kafka\produce([
        "bootstrap.servers" => "host1:port1,host2:port2",
    ], ["test"]);
    
    for($i=0;$i<100;++$i) {
        echo $i, "\n";
        $message = new flame\kafka\message("this is the payload", "this is the key");
        $message->header["key"] = "value";
        $producer->publish("test", $message);
        flame\time\sleep(50);
        $producer->publish("test", "this is the payload", "this is the key");
        flame\time\sleep(50);
    }
    $producer->flush();
});

flame\run();

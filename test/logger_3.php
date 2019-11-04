<?php
flame\init("logger_3", [
    "logger" => __DIR__."/logger_3.log"
]);

flame\go(function() {
    $logger = flame\log\connect(__DIR__."/logger_3.log.extra");
    for($i=0;$i<3;++$i) { 
        flame\log\debug("writing to default log");
        $logger->write("无前缀的日志数据：", flame\time\now());
        flame\time\sleep(rand(4000, 5000));
    }
});

flame\run();

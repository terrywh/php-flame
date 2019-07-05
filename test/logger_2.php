<?php
flame\init("logger_2", [
    "logger" => __DIR__."/logger_2.log", // 目标日志文件
]);

flame\go(function() {
    for($i=0;$i<10000;++$i) {
        flame\time\sleep(rand(4000, 5000));
        flame\log\trace("WORKING:", intval(getenv("FLAME_CUR_WORKER")));
        flame\time\sleep(rand(4000, 5000));
        // 标准、错误输出在多进程模式重定向到默认日志文件
        echo "[", flame\time\iso(), "] (TRACE) WORKING: ", intval(getenv("FLAME_CUR_WORKER")), "\n";
    }
    flame\log\trace("DONE:", intval(getenv("FLAME_CUR_WORKER")));
});

flame\on("quit", function() {
    flame\log\trace("QUITING:", intval(getenv("FLAME_CUR_WORKER")));
    // flame\quit();
});

flame\run();

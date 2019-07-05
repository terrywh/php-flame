<?php
flame\init("logger_1", [
    "logger" => __DIR__."/logger_1.log"
]);

flame\go(function() {
    flame\log\warning("This is a warning message");
    flame\time\sleep(1000);
    flame\log\warn("This is another warning message");
});

flame\run();

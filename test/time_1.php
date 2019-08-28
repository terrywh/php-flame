<?php
flame\init("time_1");

flame\go(function() {
    echo flame\time\now(), "\n";
    echo flame\time\iso(), "\n";

    flame\go(function() {
        flame\time\sleep(10000);
    });
    flame\time\sleep(10000);
});

flame\run();

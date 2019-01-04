<?php
flame\init("coroutine_1");

flame\go(function() {
    flame\go(function() {
        for($i=0;$i<1000;++$i) {
            flame\time\sleep(1);
            flame\go(function() use($i) {
                echo "x: ".flame\co_id()."\n";
                flame\time\sleep(mt_rand(10, 50));
                echo "a: $i\n";
                flame\time\sleep(mt_rand(10, 50));
                echo "b: $i\n";
                flame\time\sleep(mt_rand(10, 50));
                echo "c: $i\n";
            });
        }
        echo "done2\n";
    });
    echo "done1\n";
});

flame\run();
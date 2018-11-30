<?php
flame\init("coroutine_1");

flame\go(function() {
    // flame\go(function() {
    //     flame\time\sleep(100);
    //     echo "done.\n";
    // });
    echo "done.\n";
});

flame\run();
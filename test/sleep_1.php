<?php
flame\init("sleep_1");

function mySleep1() {
    mySleep2(rand(10, 30));
}

function mySleep2($ms) {
    flame\time\sleep($ms);
}

for($i=0;$i<200;++$i) {
    flame\go(function() {
        $i = 0;
        for($x=0;$x<2000;++$x) {
            echo ++$i, "\n";
            mySleep1();
            // flame\time\sleep(10);
        }
    });
}


flame\run();

<?php
flame\init("queue_1");

flame\go(function() {
    $q1 = new flame\queue();
    $q2 = new flame\queue();
    $q3 = new flame\queue();
    flame\go(function() use($q1) {
        for($i=1;$i<10;++$i) {
            $q1->push($i);
            flame\time\sleep(rand(50, 100));
        }
        $q1->close();
    });
    flame\go(function() use($q2) {
        for($i=1;$i<10;++$i) {
            $q2->push($i + 10);
            flame\time\sleep(rand(50, 150));
        }
        $q2->close();
    });
    flame\go(function() use($q3) {
        for($i=1;$i<10;++$i) {
            $q3->push($i + 100);
            flame\time\sleep(rand(150, 250));
        }
        $q3->close();
    });
    flame\go(function() use($q1) {
        while($x = $q1->pop()) {
            echo "(1) q1: ", $x, "\n";
            flame\time\sleep(rand(100, 200));
        }
        echo "(1) q1: consume done\n";
    });
    flame\go(function() use($q1) {
        while($x = $q1->pop()) {
            echo "(2) q1: ", $x, "\n";
            flame\time\sleep(rand(100, 200));
        }
        echo "(2) q1: consume done\n";
    });
    flame\go(function() use($q2, $q3) {
        while($q = flame\select($q2, $q3)) {
            // 必须使用 全等 符号比较对象
            if($q === $q2) {
                echo "q2: ", $q->pop(), "\n";
            }else if($q === $q3) {
                echo "q3: ", $q->pop(), "\n";
            }else{
                // 
            }
        }
        echo "q2: consume done\n";
        echo "q3: consume done\n";
    });
});

flame\run();

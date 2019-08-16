<?php
flame\init("redis_1");

flame\go(function() {
    $cli = flame\redis\connect("redis://10.110.40.140:8888/");
    echo "IPv4: 223.104.190.94 ->\n";
    var_dump( $cli->address("223.104.190.94") );
    echo "Segment: -> \n";
    var_dump( $cli->segment("中国是一个神奇的地方，网络管控不是一般的严！") );
    echo "Eemoji: 123456 (string) -> \n";
    var_dump( $cli->eemoji("123456") );
    echo "Eemoji: 123456 (integer/animal) -> \n";
    var_dump( $cli->eemoji(123456, "INTEGER", "ANIMAL") );
    echo "Eemoji: 123456 (integer/food) -> \n";
    var_dump( $cli->eemoji(123456, "INTEGER", "FOOD") );
});

flame\run();

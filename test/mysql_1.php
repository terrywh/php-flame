<?php

flame\init("mysql_1");

flame\go(function() {
    echo "1\n";
    $cli = flame\mysql\connect("mysql://ucenter_rw:eVrFSRa2NLdoIPbITvPe@rm-2ze8xu255t0523w07.mysql.rds.aliyuncs.com:3306/ucenter");
    echo "2\n";
    var_dump( $cli->escape('a.b') );
    echo "3\n";
});


flame\run();

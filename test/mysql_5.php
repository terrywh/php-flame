<?php
flame\init("mysql_5");
flame\go(function() {
	// $client = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3306/database");
	$client = yield flame\mysql\connect("mysql://ucenter_rw:eVrFSRa2NLdoIPbITvPe@rm-2ze8xu255t0523w07.mysql.rds.aliyuncs.com:3306/ucenter");
    for($i = 0; $i<1000; ++$i) {
        yield flame\time\sleep(10);
        $tx = yield $client->begin_tx();
        $rs = yield $tx->select("user","*");
        $dt = yield $rs->fetch_all();
        $rs = yield $tx->insert("user", ["uid"=>1]);
        var_dump($dt);
        yield $tx->rollback();
    }
});
flame\run();

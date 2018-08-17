<?php
flame\init("mysql_4");
flame\go(function() {
	$db = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3306/database");
	for($i=0;$i<10;++$i) {
		flame\go(function() use($db, $i) {
			for($j=0;$j<100;++$j) {
				$rs = yield $db->query("SELECT 1");
				echo "$i: ". $j. "\n";
				// yield flame\time\sleep(0);
				// echo "$i: after\n";
				// 此场景，若不进行 unset 可能会导致 query 阻塞死锁（并行量过大时）
				// unset($rs);
			}
			echo "done(". $i .")\n";
		});
	}
});
flame\run();

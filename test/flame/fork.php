<?php
flame\go(function() {
	flame\fork();
	// fork 以下的代码会同时在父子进程中进行
	flame\go(function() {
		for($i=0;$i<10;++$i) {
			$rand = mt_rand(200, 2000);
			echo "[", getmypid(), "] ", $i, " ", $rand, "\n";
			
			yield flame\time\sleep($rand);
		}
	});
});
flame\run();
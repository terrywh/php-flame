<?php
flame\init("core_0");
flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(500);
		echo "[1]: $i\n";
	}
});
flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(700);
		echo "[2]: $i\n";
	}
});
flame\run();

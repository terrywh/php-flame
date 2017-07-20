<?php

flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(300);
		echo 1, " [", time(),"]\n";
	}
});

flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(200);
		echo 2, " [", time(),"]\n";
	}
});


flame\run();
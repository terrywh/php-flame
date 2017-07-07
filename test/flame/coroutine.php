<?php

function routine1() {
	for($i=0;$i<2;++$i) {
		yield flame\time\sleep(3000);
		echo 1, " [", time(),"]\n";
		yield from routine2();
	}
}
flame\go(routine1());

function routine2() {
	for($i=0;$i<2;++$i) {
		yield flame\time\sleep(1000);
		echo 2, " [", time(),"]\n";
	}
}

function routine3() {
	for($i=0;$i<4;++$i) {
		yield flame\async(function() {
			sleep(2);
		});
		echo 3, " [", time(),"]\n";
	}
}
flame\go(routine3());

flame\run();
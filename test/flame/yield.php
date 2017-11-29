<?php
function xx() {
	$i = 0;
	for($i=0;$i<5;++$i) {
		yield flame\time\sleep(1000);
		echo "xx: ", $i, "\n";
	}
}
function yy() {
	$i = 0;
	for($i=0;$i<5;++$i) {
		yield flame\time\sleep(1000);
		echo "yy: ", $i, "\n";
	}
}

flame\init("yield_test");

flame\go(function() {
	echo "-------\n";
	yield xx();
	yield yy();
	yield flame\time\sleep(1000);
	echo "-------\n";
});

flame\run();

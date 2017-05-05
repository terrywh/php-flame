<?php
dl("flame.so");
flame\run(function() {
	echo "before async\n";
	echo yield flame\async(function($done) {
		done(null, "aaaaaaaaaa")
	}), "\n";
	echo "after yield\n";
});

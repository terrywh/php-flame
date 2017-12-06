<?php
flame\init("exception_test");
flame\log\set_output("/tmp/exception.log");
flame\go(function() {
	yield flame\time\sleep(1000);
	// throw new exception("failed to do so much thing");
	test_not_exist();
});
flame\run();

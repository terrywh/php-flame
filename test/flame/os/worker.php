<?php
flame\init("ipc_worker_test");
flame\go(function() {
	echo "++++++++++\n";
	flame\os\set_message_handler(function() {

	});
	echo "++++++++++\n";
	yield flame\time\sleep(500000);
	echo "++++++++++\n";
});
flame\run();

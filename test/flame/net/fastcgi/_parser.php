<?php
flame\init("parser_test");
flame\go(function() {
	$handler = new flame\net\fastcgi\socket_handler()
	flame\os\set_message_handler(function($message, $sock) {
		if($sock) $handler();
	});
});
flame\run();

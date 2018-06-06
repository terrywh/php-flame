<?php
flame\init("http_client_test2");

flame\go(function() {
	while(true) {
		try{
			$req = yield flame\net\http\get("http://www.baidu.com", 1);
		}catch(Exception $ex) {
			if($ex->getCode() != 28) {
				break;
			}
		}
		yield flame\time\sleep(100);
	}
});

flame\run();

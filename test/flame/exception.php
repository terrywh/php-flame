<?php
flame\init("exception_test");
flame\log\set_output("/tmp/exception.log");
class TestApp {
	function start () {		
		flame\go(function() {
			yield flame\time\sleep(100);			
			// 异步流程的错误信息需要特殊处理流程还原堆栈信息
			yield flame\do_exception(false);
			// yield flame\do_exception(true);
			// throw new exception("failed to do so much thing");
			// test_not_exist();
		});
	}
}
$app = new TestApp();
$app->start();

flame\run();

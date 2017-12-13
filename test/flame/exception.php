<?php
flame\init("exception_test");
flame\log\set_output("/tmp/exception.log");
flame\go(function() {
	yield flame\time\sleep(100);
	// 异步流程的错误信息需要特殊处理流程还原堆栈信息
	yield flame\async_exception();
	// throw new exception("failed to do so much thing");
	// test_not_exist();
});
flame\run();

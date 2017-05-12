<?php
dl("flame.so")

flame\run(function() {
	// 由于 mysqli 建立连接是同步过程，将其放入工作线程，模拟异步流程
	$db = yield flame\async(function($done) {
		$done(null, new mysqli("127.0.0.1", "panda", "E0cq031]z{1|;P9J", "test"));
	});
	// 数据库连接在长时间 idle 时会被服务端主动断开，这里将数据库对象交由
	flame\db\connection::preserve($db, 300, "ping");
	// 将 mysql 的查询过程交到工作线程中去，模拟异步流程，减少对主线程的阻塞
	// 建议：不要在 async 内外同时使用 $db 相关对象（防止出现由于工作线程和主线程同时使用引起的竞争问题）
	$data = yield flame\async(function($done) use($db) {
		$rs = $db->query("SELECT * FROM `check_cash_201602` LIMIT 10");
		$done(null, $rs->fetch_all(MYSQLI_ASSOC));
	});

	var_dump($data);
});

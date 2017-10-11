<?php
flame\init("yield detect");
// flame\go(function() {
// 	flame\time\sleep(3000); // 缺少 yield 关键字（在 3000ms 后发现）
// });
flame\go(function() {
	echo "------------\n";
	yield flame\time\sleep2(3000);
	echo "============\n";
});

flame\run();

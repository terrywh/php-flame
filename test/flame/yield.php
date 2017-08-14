<?php

// flame\go(function() {
// 	flame\time\sleep(3000); // 缺少 yield 关键字（在 3000ms 后发现）
// });
flame\go(function() {
	flame\time\sleep(300);
	flame\time\sleep(300); // 立刻发现
});
flame\go(function() {
	flame\time\sleep(300);
	flame\time\sleep(300); // 立刻发现
	yield flame\time\sleep(300);
});

flame\run();

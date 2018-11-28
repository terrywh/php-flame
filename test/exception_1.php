<?php
flame\init("exception_1");

flame\go(function() {
    flame\time\sleep(1000);
    throw new Exception("Aaaaaaa");
});

flame\run();

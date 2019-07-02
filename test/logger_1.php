<?php
flame\init("logger_1");

flame\go(function() {
    flame\log\warning("This is a warning message");
});

flame\run();
<?php
flame\init("worker");

flame\go(function() {
    $users = [];


    $process_count = intval(getenv("FLAME_MAX_WORKERS"));
    $process_index = intval(getenv("FLAME_CUR_WORKER")) - 1;

    foreach($users as $user) {
        if($user->uid % $process_count == $process_index) {
            /// ...
        }
    } 

    
});

flame\on("quit", function() {
    echo "quiting\n";
});

flame\run();

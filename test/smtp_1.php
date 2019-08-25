<?php

flame\init("smtp_1");
flame\go(function() {
    $cli = flame\smtp\connect("smtp://user:pass@smtp.mxhichina.com:25");
    // $cli = flame\smtp\connect("smtp://smtp.mxhichina.com:25");
    $msg = (new flame\smtp\message())
        ->from(["测试"=>"user"])
        ->to(["测试"=>"xxxx@xxxx.xxx"])
        ->cc(["测试"=>"xxxx@xxxx.xxx"])
        ->subject("测试 FLAME 邮件接口")
        ->append(
            file_get_contents("/path/to/embed/file/small.png"), [
                "Content-ID"   => "embed_image_1",
                "Content-Type" => "application/octet-stream",
                "Content-Disposition" => 'inline; filename="small.png"',
            ])
        ->append(
            file_get_contents("/path/to/attach/file/logo.png"), [
                "Content-Type" => "application/octet-stream",
                "Content-Disposition" => 'attachment; filename="logo.png"',
            ])
        ->append('HTML <strong>STRONG</strong><br/>EMBED：<img src="cid:embed_image_1" /><br/>', ["Content-Type" => 'text/html; charset="UTF-8"']);
    
    // echo $msg, "\n";
    var_dump( $cli->post($msg) );
});
flame\run();
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html lang="en" xml:lang="en"  xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>monotone <?php echo isset($page_title) ? " : $page_title" : "" ?></title>
<link type="text/css" rel="stylesheet" href="res/styles.css" />
<!--[if IE 7]>
<link rel="stylesheet" type="text/css" href="res/ie7.css" />
<![endif]-->
<link rel="alternate" type="application/rss+xml" title="monotone news" href="news.xml.php" />
<link rel="alternate" type="application/rss+xml" title="monotone releases" href="releases.xml.php" />
<script type="text/javascript" src="https://www.google.com/jsapi?key=ABQIAAAA19yJy5ZKGCq-pdY8NMZbrxQBE22dz7hS6Lv_ovduzW42XUZpZhQ-hDriybitASPo1d8A770Y_f5hug"></script>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.4.2/jquery.min.js"></script>
<script type="text/javascript">
$(document).ready(function() {
    $("#pageflip").hover(function() {
        $("#pageflip img , .msg_block").stop()
            .animate({
                width: '305px',
                height: '317px'
            }, 500);
        } , function() {
        $("#pageflip img").stop()
            .animate({
                width: '50px',
                height: '52px'
            }, 220);
        $(".msg_block").stop()
            .animate({
                width: '50px',
                height: '50px'
            }, 200);
    });
});
</script>
<style type="text/css">
#pageflip {
    position: relative;
}
#pageflip img {
    width: 50px; height: 52px;
    z-index: 99;
    position: absolute;
    right: 0; top: 0;
    -ms-interpolation-mode: bicubic;
}
#pageflip .msg_block {
    width: 50px; height: 50px;
    position: absolute;
    z-index: 50;
    right: 0; top: 0;
    background: url(res/teaser.png) no-repeat right top;
    text-indent: -9999px;
}
</style>
</head>
<body>

<div id="pageflip">
    <a href="#">
        <img src="res/page_flip.png" alt="" />
        <span class="msg_block">monotone 1.0 is landing soon...</span>
    </a>
</div>

<div id="header">
<p>
<a href="index.php"><img src="res/logo.png" alt="monotone logo" border="0"/></a>
<strong>monotone</strong> is a free distributed version control system.
It provides a simple, single-file transactional version store, with
fully disconnected operation and an efficient peer-to-peer
synchronization protocol. It understands history-sensitive merging,
lightweight branches, integrated code review and 3rd party testing.
It uses cryptographic version naming and client-side RSA certificates.
It has good internationalization support, runs on Linux, Solaris,
Mac OS X, Windows, and other unixes, and is licensed under the GNU GPL.
</p>
<div class="clearfloat"></div>
</div>

<div id="body">


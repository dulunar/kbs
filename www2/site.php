<?php
define("ATTACHMAXSIZE",BBS_MAXATTACHMENTSIZE);        //附件总字节数的上限，单位 bytes
define("ATTACHMAXCOUNT",BBS_MAXATTACHMENTCOUNT);      //附件数目的上限
define("MAINPAGE_FILE","mainpage.php");              //首页导读的 URL
define("QUOTED_LINES", BBS_QUOTED_LINES);             //web 回文保留的引文行数
define("SITE_NEWSMTH", 1);
define("RUNNINGTIME", 1);                             //底部显示页面运行时间
define("AUTO_BMP2JPG_THRESHOLD", 100000); // requires ImageMagick and safe_mode off

// 界面方案的名称
$style_names = array(
	"蓝色经典",
	"冬雪皑皑"
);

?>

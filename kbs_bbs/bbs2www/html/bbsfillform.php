<?php
	/**
	 * This file Fill registry form.
	 * by binxun 2003.5
	 */
	require("funcs.php");
    html_init("gb2312");
	
	if ($loginok != 1)
		html_nologin();
	else
	{
	    @$realname=$_POST["realname"];
	    @$dept=$_POST["dept"];
        @$address=$_POST["address"];
		@$year=$_POST["year"];
		@$month=$_POST["month"];
		@$day=$_POST["day"];
	    @$email=$_POST["email"];
		@$phone=$_POST["phone"];


    $ret=bbs_createregform($realname,$dept,$address,$year,$month,$day,$email,$phone);

	switch($ret)
	{
	case 0:

	default:
			html_error_quit("未知的错误!");
			break;
	}
?>
<body>
注册单已经提交,24小时内站务将会审核,如果通过,你就会获得合法用户权限！
</body>
</html>

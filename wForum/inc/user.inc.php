<?php

function showUserMailBoxOrBR() { //用于普通用户就可以访问的页面
	global $loginok;
	if ($loginok==1) {
		showUserMailBox();
	} else {
		echo "<br>";
	}
}

function showUserMailbox(){ //这个函数直接调用必须保证 $loginok==1
	global $currentuser;
?>
<table cellSpacing=0 cellPadding=0 width=97% border=0 align=center>
<tr><td width=65% >
</td><td width=35% align=right>
<?php   
	bbs_getmailnum($currentuser["userid"],$total,$unread);
	if ($unread>0)  {
?>
<bgsound src="sound/newmail.wav" border=0>
<img src="pic/msg_new_bar.gif" /> <a href="usermailbox.php?boxname=inbox">我的收件箱</a> (<a href="usermail.php?boxname=inbox&num=<?php echo $total-1;?>" target=_blank><font color="#FF0000"><?php  echo $unread; ?> 新</font></a>)
<?php   }
    else
  {
?>
<img src="pic/msg_no_new_bar.gif" /> <a href="usermailbox.php?boxname=inbox">我的收件箱</a> (<font color=gray>0 新</font>)
<?php
  }
?>
</td></tr>
</table>
<?php
}

/* $secNum == -1 means fav */
function setSecFoldCookie($secNum, $isFold) {
	if (isset($_COOKIE["ShowSecBoards"])) {
		$ssb = $_COOKIE["ShowSecBoards"];
		settype($ssb, "integer");
	}
	else $ssb = 1;
	if ($isFold) {
		$ssb = $ssb | (1 << ($secNum+1));
	} else {
		$ssb = $ssb & ~(1 << ($secNum+1));
	}
	setcookie('ShowSecBoards', $ssb ,time() + 604800);
	$_COOKIE["ShowSecBoards"] = $ssb;
	return 0;
}

function getSecFoldCookie($secNum) {
	if (isset($_COOKIE["ShowSecBoards"])) {
		$ssb = $_COOKIE["ShowSecBoards"];
		settype($ssb, "integer");
	}
	else $ssb = 1;
	return (($ssb & (1 << ($secNum+1))) != 0);
}

function showSecsJS($secNum,$group,$isFold,$isFav) {
	global $yank;
	global $section_nums;
	
	if ($isFav) {
		$select = $secNum; //代码相似性 :D
	}
?>
<script language="JavaScript">
<!--
	boards = new Array();
	select = npos = bid = isUnread = lastID = lastTitle = lastOwner = lastPosttime = bm = 0;
<?php
	if (!$isFold && (BOARDLISTSTYLE=='simplest') && !$isFav) {
		// hehe 
	} else {	
		if (!$isFav) {
			$boards = bbs_getboards($section_nums[$secNum], $group, $yank);
		} else {
			$boards = bbs_fav_boards($select, 1);
			if ($boards == FALSE) {
	    		//foundErr("读取版列表失败");
			}
		}
		if ($boards !== FALSE) {
			$brd_name = $boards["NAME"]; // 英文名
			$brd_desc = $boards["DESC"]; // 中文描述
			$brd_class = $boards["CLASS"]; // 版分类名
			$brd_bm = $boards["BM"]; // 版主
			$brd_artcnt = $boards["ARTCNT"]; // 文章数
			$brd_unread = $boards["UNREAD"]; // 未读标记
			$brd_zapped = $boards["ZAPPED"]; // 是否被 z 掉
			if ($isFav) {
                $brd_position= $boards["POSITION"];//位置
                $brd_npos= $boards["NPOS"];//位置
            }
			$brd_flag = $boards["FLAG"]; //flag
			$brd_bid = $boards["BID"]; //flag
			$rows = sizeof($brd_name);
			$showed = 0;
			for ($i = 0; $i < $rows; $i++)	{
				if ($brd_name[$i]=='Registry')
					continue;
				if ($brd_name[$i]=='bbsnet')
					continue;
				if ($brd_name[$i]=='undenypost')
					continue;
				
				if ($isFav && ($brd_bid[$i] == -1)) {
					continue;
				}				

				$isGroup = ((!$isFav) && ($brd_flag[$i] & BBS_BOARD_GROUP)) || ($isFav && ($brd_flag[$i] == -1));
				echo "isGroup = ".($isGroup?"true":"false").";\n";
				echo "boardDesc = '".addslashes(htmlspecialchars($brd_desc[$i], ENT_QUOTES))."';\n";
				if ($isGroup && $isFav) {
					echo "boardName = boardDesc;\n";
				} else {
					echo "boardName = '".$brd_name[$i]."';\n";
				}

				if ($isGroup) {
					echo "todayNum = nThreads = 0;\n";
				} else {
					echo "todayNum = ".bbs_get_today_article_num($brd_name[$i]).";\n";
					echo "nThreads = ".bbs_getthreadnum($brd_bid[$i]).";\n";
				}
				if ($isGroup) {
					$nArticles = 0;
				} else {
					$nArticles = $brd_artcnt[$i];
				}
				echo "nArticles = $nArticles;\n";
				if ($isFav) {
					echo "select = ".$select.";\n";
					echo "npos = ".$brd_npos[$i].";\n";
					echo "bid = ".$brd_bid[$i].";\n";
				}
				if ($isFold) {
					echo "isUnread = ".($brd_unread[$i] == 1 ? "true" : "false").";\n";
					if ($nArticles > 0) {
						bbs_getthreadnum($brd_bid[$i]); //ToDo: this is only dirty fix: 触发必要的 .WEBTHREAD 更新
						$articles = bbs_getthreads($brd_name[$i], 0, 1,0 ); //$brd_artcnt[$i], 1, $default_dir_mode);
						if ($articles == FALSE) {
							$nArticles = 0;
						} else {
							$nArticles = $brd_artcnt[$i];
							$lastID = $articles[0]['origin']['ID'];
							$lastTitle = addslashes(htmlspecialchars($articles[0]['origin']['TITLE'],ENT_QUOTES));
							$lastOwner = $articles[0]['origin']['OWNER'];
							$lastPosttime = strftime('%Y-%m-%d %H:%M:%S', intval($articles[0]['origin']['POSTTIME']));
						}
						echo "lastID = $lastID;\n";
						echo "lastTitle = '$lastTitle';\n";
						echo "lastOwner = '$lastOwner';\n";
						echo "lastPosttime = '$lastPosttime';\n";
					}
					echo "bm = '".$brd_bm[$i]."';\n";
				}
				echo "boards[boards.length] = new Board(isGroup,isUnread,boardName,boardDesc,lastID,lastTitle,lastOwner,lastPosttime,bm,todayNum,nArticles,nThreads,select,npos,bid);\n";
			}
		}
	}
?>
	boards<?php echo $secNum; ?> = boards;
	fold<?php echo $secNum; ?> = <?php echo ($isFold?"true":"false"); ?>;
//-->
</script>
<?php
}


/*
 * $isFav = true 的时候表示载入收藏夹，参数说明：$secNum 相当于 bbs2www/html/bbsfav.php 里头的 $select, $group 参数此时没有作用 - atppp
 * isFold: true when show detailed boards list.
 */
function showSecs($secNum,$group,$isFold,$isFav = false) {
	global $section_names;
?>
<table cellspacing=0 cellpadding=0 align=center width="97%" class=TableBorder1>
<TR><Th height=25 align=left id=TableTitleLink>&nbsp;
<?php
	if (!$isFav) {
		if ($group == 0) {
			if ($isFold) {
?>
<img src="pic/nofollow.gif" id="followImg<?php echo $secNum; ?>" style="cursor:hand;" onclick="loadSecFollow(<?php echo $secNum; ?>)" border=0 title="关闭版面列表">
<?php
			} else {
?>
<img src="pic/plus.gif" id="followImg<?php echo $secNum; ?>" style="cursor:hand;" onclick="loadSecFollow(<?php echo $secNum; ?>)" border=0 title="展开版面列表">
<?php
			}
		} else {
?>
<img src="pic/nofollow.gif" border=0>
<?php
		}
?>
<a href="section.php?sec=<?php echo $secNum ; ?>" title=进入本分类讨论区><?php echo $section_names[$secNum][0]; ?></a>
<?php
	} else {
		$select = $secNum; //代码相似性 :D
		if ($isFold) {
?>
<img src="pic/nofollow.gif" id="followImg<?php echo $secNum; ?>" style="cursor:hand;" onclick="loadFavFollow(<?php echo $select; ?>)" border=0 title="关闭版面列表">
<?php
		} else {
?>
<img src="pic/plus.gif" id="followImg<?php echo $secNum; ?>" style="cursor:hand;" onclick="loadFavFollow(<?php echo $select; ?>)" border=0 title="展开版面列表">
<?php
		}
?>
用户收藏夹
<?php
		if ($select != 0) {
			$list_father = bbs_get_father($select);
?>
&nbsp;[<a href="favboard.php?select=<?php echo $list_father; ?>">回到上一级</a>]
<?php
		}
?>
&nbsp;[<a href="modifyfavboards.php?select=<?php echo $select; ?>">管理本收藏夹目录</a>]
<?php
	}
?>
</th></tr>
<?php
	showSecsJS($secNum,$group,$isFold,$isFav);
?>
<tr style="display:none" id="followTip<?php echo $secNum; ?>"><td style="text-align:center;border:1px solid black;background-color:lightyellow;color:black;padding:2px" onclick="loadSecFollow('<?php echo $secNum; ?>')">正在读取版面列表数据，请稍侯……</div></td></tr>
<TR><Td id="followSpan<?php echo $secNum; ?>">
<script language="JavaScript">
<!--
	str = showSec(<?php echo ($isFold?"true":"false"); ?>, <?php echo ($isFav?"true":"false"); ?>, boards, <?php echo $secNum ?>);
	document.write(str);
//-->
</script>
</td></tr>
</table><br>
<?php
}

function showAnnounce(){
	global $AnnounceBoard;
	global $SiteName;
?>
<tr>
<td align=center width=100% valign=middle colspan=2>
<link rel="stylesheet" type="text/css" href="css/fader.css">
<SCRIPT LANGUAGE='JavaScript' SRC='inc/fader.js' TYPE='text/javascript'></script>
<SCRIPT LANGUAGE='JavaScript' TYPE='text/javascript'>
prefix="";
arNews = [<?php 
		$brdarr = array();
		$brdnum = bbs_getboard($AnnounceBoard, $brdarr);
		if ( ($brdnum==0) || ($brdarr["FLAG"] & BBS_BOARD_GROUP) ) {
			echo '"当前没有公告","",';
		} else {
			$total = bbs_getThreadNum($brdnum);
			if ($total <= 0) {
				echo '"当前没有公告","",';
			} else {
				$articles = bbs_getthreads($brdarr['NAME'], 0, ANNOUNCENUMBER,1); 
				if ($articles == FALSE) {
					echo '"当前没有公告","",';
				} else {
					$num=count($articles);
					for ($i=0;$i<$num;$i++) {
					echo "'<b><a href=\"disparticle.php?boardName=".$brdarr['NAME']."&ID=".$articles[$i]['origin']['ID']."\">" .htmlspecialchars($articles[$i]['origin']['TITLE'],ENT_QUOTES) . "</a></b> (".strftime('%Y-%m-%d %H:%M:%S', intval($articles[$i]['origin']['POSTTIME'])).")',\"\",";
					}
				}
			}
		}
?>"<b>欢迎光临<?php echo $SiteName; ?></b>",""
];
</SCRIPT>
<div id="elFader" style="position:relative;visibility:hidden; height:16" ></div>
</td>
</tr>
<?php
}

function usersysinfo($info){
	if (USEBROWSCAP == 0) { //FireFox, Opera 都判断不对 - atppp
		if (strpos($info,';')!==false)  {
			$usersys=explode(';',$info);
			if (count($usersys)>=2)  {
				$usersys[1]=str_replace("MSIE","Internet Explorer",$usersys[1]);
				$usersys[2]=str_replace(")","",$usersys[2]);
				$usersys[2]=str_replace("NT 5.1","XP",$usersys[2]);
				$usersys[2]=str_replace("NT 5.0","2000",$usersys[2]);
				$usersys[2]=str_replace("9x","Me",$usersys[2]);
				$usersys[1]="浏 览 器：".trim($usersys[1]);
				$usersys[2]="操作系统：".trim($usersys[2]);
				$function_ret=$usersys[1].'，'.$usersys[2];
			}  else  {
				$function_ret='浏 览 器：未知，操作系统：未知';
			}
		} else {
			$function_ret="未知，未知";
		}
	} else {
		$browser = get_browser($info);
		$str1 = $browser->parent;
		$str1 = str_replace("IE","Internet Explorer",$str1);
		if ($str1 == "") $str1 = "未知";
		$str2 = $browser->platform;
		$str2 = str_replace("Win","Windows ",$str2);
		if ($str2 == "") $str2 = "未知";
		$function_ret = "浏 览 器：".$str1."，操作系统：".$str2;
	}
	return $function_ret;
} 

function showUserInfo(){
?>
<table cellpadding=5 cellspacing=1 class=TableBorder1 align=center style="word-break:break-all;" width="97%">
<TR><Th align=left colSpan=2 height=25>-=&gt; 用户来访信息</Th></TR>
<TR><TD vAlign=top class=TableBody1 height=25 width=100% >
<?php
$userip = $_SERVER["HTTP_X_FORWARDED_FOR"];
$userip2 = $_SERVER["REMOTE_ADDR"];
if ($userip=='') $userip = $userip2;
echo '您的真实ＩＰ是：'. $userip. '，';
echo usersysinfo($_SERVER["HTTP_USER_AGENT"]);

?>
</TD></TR></table><br>
<?php

}

function showOnlineUsers(){
?>
<table cellpadding=5 cellspacing=1 class=TableBorder1 align=center style="word-break:break-all;" width="97%">
<TR><Th colSpan=2 align=left id=TableTitleLink height=25 style="font-weight:normal">
	<b>-=&gt; 论坛在线统计</b>&nbsp;[<a href=showonlineuser.php>显示详细列表</a>]
	<!--[<a href=boardstat.php?reaction=online>查看在线用户位置</a>]-->
</Th></TR>
<TR><TD width=100% vAlign=top class=TableBody1>  目前论坛上总共有 <b><?php echo bbs_getonlinenumber() ; ?></b> 人在线，其中注册用户 <b><?php echo bbs_getonlineusernumber(); ?></b> 人，访客 <b><?php echo bbs_getwwwguestnumber() ; ?></b> 人。<!--<br>
历史最高在线纪录是 <b> </b> 人同时在线-->
</td></tr>
</table><br>
<?php
}

function showSample(){
?>
<table cellspacing=1 cellpadding=3 width="97%" border=0 align=center>
<tr><td align=center><img src="pic/forum_nonews.gif" align="absmiddle">&nbsp;没有新的帖子&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="pic/forum_isnews.gif" align="absmiddle">&nbsp;有新的帖子&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="pic/forum_lock.gif" align="absmiddle">&nbsp;被锁定的论坛</td></tr>
</table><br>
<?php
}

function showMailSampleIcon(){
?>
<table cellspacing=1 cellpadding=3 width="97%" border=0 align=center>
<tr><td align=center><img src="pic/m_news.gif" align="absmiddle">&nbsp;未读邮件&nbsp<img src="pic/m_olds.gif" align="absmiddle">&nbsp;已读邮件&nbsp<img src="pic/m_replys.gif" align="absmiddle">&nbsp;已回复邮件&nbsp;<img src="pic/m_newlocks.gif" align="absmiddle">&nbsp;锁定的未读邮件&nbsp;<img src="pic/m_oldlocks.gif" align="absmiddle">&nbsp;锁定的已读邮件&nbsp;<img src="pic/m_lockreplys.gif" align="absmiddle">&nbsp;锁定的已回复邮件</td></tr>
</table><br>
<?php
}
?>

/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* 所有 的注释 由 Alex&Sissi 添加 ， alex@mars.net.edu.cn */

#include "bbs.h"
#include <time.h>

/*#include "../SMTH2000/cache/cache.h"*/

extern int numofsig;
int scrint = 0;
int local_article;
int readpost;
int digestmode;
int usernum;
char currboard[STRLEN - BM_LEN];
char currBM[BM_LEN - 1];
int selboard = 0;

char ReadPost[STRLEN] = "";
char ReplyPost[STRLEN] = "";
struct fileheader ReadPostHeader;
int Anony;
char genbuf[1024];
unsigned int tmpuser = 0;
char quote_title[120], quote_board[120];
char quote_user[120];
extern char currdirect[255];

#ifndef NOREPLY
char replytitle[STRLEN];
#endif

char *filemargin();

/*For read.c*/
int change_post_flag(char *currBM, struct userec *currentuser, int digestmode, char *currboard, int ent, struct fileheader *fileinfo, char *direct, int flag, int prompt);

/* bad 2002.8.1 */

int auth_search_down();
int auth_search_up();
int t_search_down();
int t_search_up();
int post_search_down();
int post_search_up();
int thread_up();
int thread_down();
int deny_user();

/*int     b_jury_edit();  stephen 2001.11.1*/
int add_author_friend();
int m_read();                   /*Haohmaru.2000.2.25 */
int SR_first_new();
int SR_last();
int SR_first();
int SR_read();
int SR_readX();                 /* Leeward 98.10.03 */
int SR_author();
int SR_authorX();               /* Leeward 98.10.03 */
int SR_BMfunc();
int SR_BMfuncX();               /* Leeward 98.04.16 */
int Goodbye();
int i_read_mail();              /* period 2000.11.12 */

void RemoveAppendedSpace();     /* Leeward 98.02.13 */
int set_delete_mark(int ent, struct fileheader *fileinfo, char *direct);        /* KCN */

extern time_t login_start_time;
extern int cmpbnames();
extern int B_to_b;

extern struct screenline *big_picture;
extern struct userec *user_data;

int check_readonly(char *checked)
{                               /* Leeward 98.03.28 */
    if (checkreadonly(checked)) {       /* Checking if DIR access mode is "555" */
        if (currboard == checked) {
            move(0, 0);
            clrtobot();
            move(8, 0);
            prints("                                        "); /* 40 spaces */
            move(8, (80 - (24 + strlen(checked))) / 2); /* Set text in center */
            prints("[1m[33m很抱歉：[31m%s 版目前是只读模式[33m\n\n                          您不能在该版发表或者修改文章[m\n", checked);
            pressreturn();
            clear();
        }
        return true;
    } else
        return false;
}

int insert_func(int fd, struct fileheader *start, int ent, int total, struct fileheader *data, bool match)
{
    int i;
    struct fileheader UFile;

    if (match)
        return 0;
    UFile = start[total - 1];
    for (i = total - 1; i >= ent; i--)
        start[i] = start[i - 1];
    lseek(fd, 0, SEEK_END);
    if (safewrite(fd, &UFile, sizeof(UFile)) == -1)
        bbslog("user", "%s", "apprec write err!");
    start[ent - 1] = *data;
    return ent;
}

/* undelete 一篇文章 Leeward 98.05.18 */
/* modified by ylsdd */
int UndeleteArticle(int ent, struct fileheader *fileinfo, char *direct)
{
    char *p, buf[1024];
    char UTitle[128];
    struct fileheader UFile;
    int i;
    FILE *fp;
    int fd;

    if (digestmode != 4 && digestmode != 5)
        return DONOTHING;
    if (!chk_currBM(currBM, currentuser))
        return DONOTHING;

    sprintf(buf, "boards/%s/%s", currboard, fileinfo->filename);
    if (!dashf(buf)) {
        clear();
        move(2, 0);
        prints("该文章不存在，已被恢复, 删除或列表出错");
        pressreturn();
        return FULLUPDATE;
    }
    fp = fopen(buf, "r");
    if (!fp)
        return DONOTHING;


    strcpy(UTitle, fileinfo->title);
    if ((p = strrchr(UTitle, '-')) != NULL) {   /* create default article title */
        *p = 0;
        for (i = strlen(UTitle) - 1; i >= 0; i--) {
            if (UTitle[i] != ' ')
                break;
            else
                UTitle[i] = 0;
        }
    }

    i = 0;
    while (!feof(fp) && i < 2) {
        fgets(buf, 1024, fp);
        if (feof(fp))
            break;
        if (strstr(buf, "发信人: ") && strstr(buf, "), 信区: ")) {
            i++;
        } else if (strstr(buf, "标  题: ")) {
            i++;
            strcpy(UTitle, buf + 8);
            if ((p = strchr(UTitle, '\n')) != NULL)
                *p = 0;
        }
    }
    fclose(fp);

    bzero(&UFile, sizeof(UFile));
    strcpy(UFile.owner, fileinfo->owner);
    strcpy(UFile.title, UTitle);
    strcpy(UFile.filename, fileinfo->filename);
    UFile.attachment=fileinfo->attachment;
    UFile.accessed[0]=fileinfo->accessed[0];
    UFile.accessed[1]=fileinfo->accessed[1]&(~FILE_DEL);

    if (UFile.filename[1] == '/')
        UFile.filename[2] = 'M';
    else
        UFile.filename[0] = 'M';
    UFile.id = fileinfo->id;
    UFile.groupid = fileinfo->groupid;
    UFile.reid = fileinfo->reid;
    set_posttime2(&UFile, fileinfo);

    setbfile(genbuf, currboard, fileinfo->filename);
    setbfile(buf, currboard, UFile.filename);
    f_mv(genbuf, buf);

    sprintf(buf, "boards/%s/.DIR", currboard);
    if ((fd = open(buf, O_RDWR | O_CREAT, 0644)) != -1) {
        if ((UFile.id == 0) || mmap_search_apply(fd, &UFile, insert_func) == 0) {
            flock(fd, LOCK_EX);
            if (UFile.id == 0) {
                UFile.id = get_nextid(currboard);
                UFile.groupid = UFile.id;
                UFile.groupid = UFile.id;
            }
            lseek(fd, 0, SEEK_END);
            if (safewrite(fd, &UFile, sizeof(UFile)) == -1)
                bbslog("user", "%s", "apprec write err!");
            flock(fd, LOCK_UN);
        }
        close(fd);
    }

    updatelastpost(currboard);
    fileinfo->filename[0] = '\0';
    substitute_record(direct, fileinfo, sizeof(*fileinfo), ent);
    sprintf(buf, "undeleted %s's “%s” on %s", UFile.owner, UFile.title, currboard);
    bbslog("user", "%s", buf);

    clear();
    move(2, 0);
    prints("'%s' 已恢复到版面 \n", UFile.title);
    pressreturn();
    bmlog(currentuser->userid, currboard, 9, 1);

    return FULLUPDATE;
}

int check_stuffmode()
{
    if (uinfo.mode == RMAIL)
        return true;
    else
        return false;
}

/*Add by SmallPig*/
void setqtitle(char *stitle)
{                               /* 取 Reply 文章后新的 文章title */
    if (strncmp(stitle, "Re: ", 4) != 0 && strncmp(stitle, "RE: ", 4) != 0) {
        snprintf(ReplyPost,STRLEN, "Re: %s", stitle);
        ReplyPost[STRLEN]=0;
        strncpy(ReadPost, stitle, STRLEN - 1);
        ReadPost[STRLEN - 1] = 0;
    } else {
        strncpy(ReplyPost, stitle, STRLEN - 1);
        strncpy(ReadPost, ReplyPost + 4, STRLEN - 1);
        ReplyPost[STRLEN - 1] = 0;
        ReadPost[STRLEN - 1] = 0;
    }
}

/*Add by SmallPig*/
int shownotepad()
{                               /* 显示 notepad */
    modify_user_mode(NOTEPAD);
    ansimore("etc/notepad", true);
    clear();
    return 1;
}

void printutitle()
{                               /* 屏幕显示 用户列表 title */
    /*---	modified by period	2000-11-02	hide posts/logins	---*/
#ifndef _DETAIL_UINFO_
    int isadm;
    const char *fmtadm = "#上站 #文章", *fmtcom = "           ";

    isadm = HAS_PERM(currentuser, PERM_ADMINMENU);
#endif

    move(2, 0);
    clrtoeol();
    prints(
#ifdef _DETAIL_UINFO_
              "[44m 编 号  使用者代号     %-19s  #上站 #文章 %4s    最近光临日期   [m\n",
#else
              "[44m 编 号  使用者代号     %-19s  %11s %4s    最近光临日期   [m\n",
#endif
#if defined(ACTS_REALNAMES)
              "真实姓名",
#else
              "使用者昵称",
#endif
#ifndef _DETAIL_UINFO_
              isadm ? fmtadm : fmtcom,
#endif
              "身份");
}


int g_board_names(struct boardheader *fhdrp,void* arg)
{
    if (check_read_perm(currentuser, fhdrp)) {
        AddNameList(fhdrp->filename);
    }
    return 0;
}

void make_blist()
{                               /* 所有版 版名 列表 */
    CreateNameList();
    apply_boards((int (*)()) g_board_names,NULL);
}

int Select()
{
    modify_user_mode(SELECT);
    do_select(0, NULL, genbuf);
    return 0;
}

int Post()
{                               /* 主菜单内的 在当前版 POST 文章 */
    if (!selboard) {
        prints("\n\n先用 (S)elect 去选择一个讨论区。\n");
        pressreturn();          /* 等待按return键 */
        clear();
        return 0;
    }
#ifndef NOREPLY
    *replytitle = '\0';
#endif
    do_post();
    return 0;
}

int get_a_boardname(char *bname, char *prompt)
{                               /* 输入一个版名 */
    /*
     * struct boardheader fh; 
     */

    make_blist();
    namecomplete(prompt, bname);        /* 可以自动搜索 */
    if (*bname == '\0') {
        return 0;
    }
    /*---	Modified by period	2000-10-29	---*/
    if (getbnum(bname) <= 0)
        /*---	---*/
    {
        move(1, 0);
        prints("错误的讨论区名称\n");
        pressreturn();
        move(1, 0);
        return 0;
    }
    return 1;
}

/* Add by SmallPig */
int do_cross(int ent, struct fileheader *fileinfo, char *direct)
{                               /* 转贴 一篇 文章 */
    char bname[STRLEN];
    char dbname[STRLEN];
    char ispost[10];
    char q_file[STRLEN];

    if (!HAS_PERM(currentuser, PERM_POST)) {    /* 判断是否有POST权 */
        return DONOTHING;
    }
#ifndef NINE_BUILD
    if ((fileinfo->accessed[0] & FILE_FORWARDED) && !HAS_PERM(currentuser, PERM_SYSOP)) {
        clear();
        move(1, 0);
        prints("本文章已经转贴过一次，无法再次转贴");
        move(2, 0);
        pressreturn();
        return FULLUPDATE;
    }
#endif
    if (uinfo.mode != RMAIL)
        sprintf(q_file, "boards/%s/%s", currboard, fileinfo->filename);
    else
        setmailfile(q_file, currentuser->userid, fileinfo->filename);
    strcpy(quote_title, fileinfo->title);

    clear();
#ifndef NINE_BUILD
    move(4, 0);                 /* Leeward 98.02.25 */
    prints
        ("[1m[33m请注意：[31m本站站规规定：同样内容的文章严禁在 5 (含) 个以上讨论区内重复张贴。\n\n违反者[33m除所贴文章会被删除之外，还将被[31m剥夺继续发表文章的权力。[33m详细规定请参照：\n\n    Announce 版的站规：“关于转贴和张贴文章的规定”。\n\n请大家共同维护 BBS 的环境，节省系统资源。谢谢合作。\n\n[m");
#endif
    move(1, 0);
    if (!get_a_boardname(bname, "请输入要转贴的讨论区名称: ")) {
        return FULLUPDATE;
    }
    /*
     * if (!strcmp(bname,currboard))Haohmaru,98.10.03 
     */
#ifndef NINE_BUILD
    if (!strcmp(bname, currboard) && (uinfo.mode != RMAIL)) {
        move(3, 0);
        clrtobot();
        prints("\n\n                          本版的文章不需要转贴到本版!");
        pressreturn();
        clear();
        return FULLUPDATE;
    }
#endif
    {                           /* Leeward 98.01.13 检查转贴者在其欲转到的版面是否被禁止了 POST 权 */
        char szTemp[STRLEN];
        struct boardheader* bh;

        strcpy(szTemp, currboard);      /* 保存当前版面 */
        strcpy(currboard, bname);       /* 设置当前版面为要转贴到的版面 */

        bh=getbcache(bname);
        if ((fileinfo->attachment!=0)&&!(bh->flag&BOARD_ATTACH)) {
            move(3, 0);
            clrtobot();
            prints("\n\n                很抱歉，该版面不能发表带附件的文章...\n");
            pressreturn();
            clear();
            strcpy(currboard, szTemp);  /* 恢复当前版面 */
            return FULLUPDATE;
        }
        if (deny_me(currentuser->userid, currboard) && !HAS_PERM(currentuser, PERM_SYSOP)) {    /* 版主禁止POST 检查 */
            move(3, 0);
            clrtobot();
            prints("\n\n                很抱歉，你在该版被其版主停止了 POST 的权力...\n");
            pressreturn();
            clear();
            strcpy(currboard, szTemp);  /* 恢复当前版面 */
            return FULLUPDATE;
        } else if (true == check_readonly(currboard)) { /* Leeward 98.03.28 */
            strcpy(currboard, szTemp);  /* 恢复当前版面 */
            return FULLUPDATE;
        } else
            strcpy(currboard, szTemp);  /* 恢复当前版面 */
    }

    move(0, 0);
    prints("转贴 ' %s ' 到 %s 版 ", quote_title, bname);
    clrtoeol();
    move(1, 0);
    getdata(1, 0, "(S)转信 (L)本站 (A)取消? [A]: ", ispost, 9, DOECHO, NULL, true);
    if (ispost[0] == 's' || ispost[0] == 'S' || ispost[0] == 'L' || ispost[0] == 'l') {
	int olddigestmode;
	/*add by stiger*/
	if(POSTFILE_BASENAME(fileinfo->filename)[0]=='Z'){
            struct fileheader xfh;
            int i,fd;
            if ((fd = open(direct, O_RDONLY, 0)) != -1) {
                for (i = ent; i > 0; i--) {
                    if (0 == get_record_handle(fd, &xfh, sizeof(xfh), i)) {
                        if (0 == strcmp(xfh.filename, fileinfo->filename)) {
                            ent = i;
                            break;
                        }
                    }
                }
                close(fd);
            }
	    if (i==0){
                move(2, 0);
	        prints("文章列表发生变化，取消");
		move(2,0);
		pressreturn();
		return FULLUPDATE;
	    }
	}
	/*add old*/
	olddigestmode=digestmode;
	digestmode=0;
        if (post_cross(currentuser, bname, currboard, quote_title, q_file, Anony, in_mail, ispost[0], 0) == -1) { /* 转贴 */
            pressreturn();
            move(2, 0);
            return FULLUPDATE;
        }
	digestmode=olddigestmode;
        move(2, 0);
        prints("' %s ' 已转贴到 %s 版 \n", quote_title, bname);
        fileinfo->accessed[0] |= FILE_FORWARDED;        /*added by alex, 96.10.3 */
        substitute_record(direct, fileinfo, sizeof(*fileinfo), ent);
    } else {
        prints("取消");
    }
    move(2, 0);
    pressreturn();
    return FULLUPDATE;
}


void readtitle()
{                               /* 版内 显示文章列表 的 title */
    struct boardheader *bp;
    char header[STRLEN*2], title[STRLEN];
    char readmode[10];
    int chkmailflag = 0;

    bp = getbcache(currboard);
    memcpy(currBM, bp->BM, BM_LEN - 1);
    if (currBM[0] == '\0' || currBM[0] == ' ') {
        strcpy(header, "诚征版主中");
    } else {
        if (HAS_PERM(currentuser, PERM_OBOARDS)) {
            char *p1, *p2;

            strcpy(header, "版主: ");
            p1 = currBM;
            p2 = p1;
            while (1) {
                if ((*p2 == ' ') || (*p2 == 0)) {
                    int end;

                    end = 0;
                    if (p1 == p2) {
                        if (*p2 == 0)
                            break;
                        p1++;
                        p2++;
                        continue;
                    }
                    if (*p2 == 0)
                        end = 1;
                    *p2 = 0;
                    if (apply_utmp(NULL, 1, p1, NULL)) {
                        sprintf(genbuf, "\x1b[32m%s\x1b[33m ", p1);
                        strcat(header, genbuf);
                    } else {
                        strcat(header, p1);
                        strcat(header, " ");
                    }
                    if (end)
                        break;
                    p1 = p2 + 1;
                    *p2 = ' ';
                }
                p2++;
            }
        } else {
            sprintf(header, "版主: %s", currBM);
        }
    }
    chkmailflag = chkmail();
    if (chkmailflag == 2)       /*Haohmaru.99.4.4.对收信也加限制 */
        strcpy(title, "[您的信箱超过容量,不能再收信!]");
    else if (chkmailflag)       /* 信件检查 */
        strcpy(title, "[您有信件]");
    else if ((bp->flag & BOARD_VOTEFLAG))       /* 投票检查 */
        sprintf(title, "投票中，按 V 进入投票");
    else
        strcpy(title, bp->title + 13);

    showtitle(header, title);   /* 显示 第一行 */
    update_endline();
    move(1, 0);
    clrtoeol();
    if (DEFINE(currentuser, DEF_HIGHCOLOR))
        prints
            ("离开[\x1b[1;32m←\x1b[m,\x1b[1;32me\x1b[m] 选择[\x1b[1;32m↑\x1b[m,\x1b[1;32m↓\x1b[m] 阅读[\x1b[1;32m→\x1b[m,\x1b[1;32mr\x1b[m] 发表文章[\x1b[1;32mCtrl-P\x1b[m] 砍信[\x1b[1;32md\x1b[m] 备忘录[\x1b[1;32mTAB\x1b[m] 求助[\x1b[1;32mh\x1b[m][m\n");
    else
        prints("离开[←,e] 选择[↑,↓] 阅读[→,r] 发表文章[Ctrl-P] 砍信[d] 备忘录[TAB] 求助[h]\x1b[m\n");
    if (digestmode == 0)        /* 阅读模式 */
        strcpy(readmode, "一般");
    else if (digestmode == 1)
        strcpy(readmode, "文摘");
    else if (digestmode == 2)
        strcpy(readmode, "主题");
    else if (digestmode == 3)
        strcpy(readmode, "精华");
    else if (digestmode == 4)
        strcpy(readmode, "回收");
    else if (digestmode == 5)
        strcpy(readmode, "纸娄");
    else if (digestmode == 6)
        strcpy(readmode, "原作");
    else if (digestmode == 7)
        strcpy(readmode, "作者");
    else if (digestmode == 8)
        strcpy(readmode, "标题");

    if (DEFINE(currentuser, DEF_HIGHCOLOR))
        prints("[1;37m[44m 编号   %-12s %6s %-40s[%4s模式] [m", "刊 登 者", "日  期", " 文章标题", readmode);
    else
        prints("[37m[44m 编号   %-12s %6s %-40s[%4s模式] [m", "刊 登 者", "日  期", " 文章标题", readmode);
    clrtoeol();
}

char *readdoent(char *buf, int num, struct fileheader *ent)
{                               /* 在文章列表中 显示 一篇文章标题 */
    time_t filetime;
    char date[20];
    char *TITLE;
    int type;
    int manager;
    char *typeprefix;
    char *typesufix;
    char attachch;

    typesufix = typeprefix = "";

    manager = chk_currBM(currBM, currentuser);

    type = get_article_flag(ent, currentuser, currboard, manager);
    if (manager && (ent->accessed[0] & FILE_IMPORTED)) {        /* 文件已经被收入精华区 */
        if (type == ' ') {
            typeprefix = "\x1b[42m";
            typesufix = "\x1b[m";
        } else {
            typeprefix = "\x1b[32m";
            typesufix = "\x1b[m";
        }
    }
    filetime = get_posttime(ent);
    if (filetime > 740000000) {
#ifdef HAVE_COLOR_DATE
        struct tm *mytm;
        char *datestr = ctime(&filetime) + 4;

        mytm = localtime(&filetime);
        sprintf(date, "[1;%dm%6.6s[m", mytm->tm_wday + 31, datestr);
#else
        strncpy(date, ctime(&filetime) + 4, 6);
        date[6] = 0;
#endif
    }
    /*
     * date = ctime( &filetime ) + 4;   时间 -> 英文 
     */
    else
        /*
         * date = ""; char *类型变量, 可能错误, modified by dong, 1998.9.19 
         */
        /*
         * { date = ctime( &filetime ) + 4; date = ""; } 
         */
        date[0] = 0;

    /*
     * Re-Write By Excellent 
     */
    if (ent->attachment!=0)
        attachch='@';
    else
        attachch=' ';
    TITLE = ent->title;         /*文章标题TITLE */
    if ((type=='d')||(type=='D')) { //置顶文章
        sprintf(buf, " \x1b[1;33m[提示]\x1b[m %-12.12s %s %c● %-44.44s ", ent->owner, date, attachch, TITLE);
        return buf;
    }

    if (uinfo.mode != RMAIL && digestmode != 1 && digestmode != 4 && digestmode != 5) { /* 新方法比较*/
            if ((ent->groupid != ent->id)&&(digestmode==DIR_MODE_THREAD||!strncasecmp(TITLE,"Re:",3)||!strncmp(TITLE,"回复:",5))) {      /*Re的文章 */
                if (ReadPostHeader.groupid == ent->groupid)     /* 当前阅读主题 标识 */
                    if (DEFINE(currentuser, DEF_HIGHCOLOR))
                        sprintf(buf, " [1;36m%4d[m %s%c%s %-12.12s %s[1;36m.%c%-47.47s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
                    else
                        sprintf(buf, " [36m%4d[m %s%c%s %-12.12s %s[36m.%c%-47.47s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
                else
                    sprintf(buf, " %4d %s%c%s %-12.12s %s %c%-47.47s", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
            } else {
                if (ReadPostHeader.groupid == ent->groupid)     /* 当前阅读主题 标识 */
                    if (DEFINE(currentuser, DEF_HIGHCOLOR))
                        sprintf(buf, " [1;33m%4d[m %s%c%s %-12.12s %s[1;33m.%c● %-44.44s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
                    else
                        sprintf(buf, " [33m%4d[m %s%c%s %-12.12s %s[33m.%c● %-44.44s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
                else
                    sprintf(buf, " %4d %s%c%s %-12.12s %s %c● %-44.44s ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
            }

    } else                     /* 允许 相同主题标识 */
        if (!strncmp("Re:", ent->title, 3)) {   /*Re的文章 */
            if (!strncmp(ReplyPost + 3, ent->title + 3,STRLEN-3)) /* 当前阅读主题 标识 */
                if (DEFINE(currentuser, DEF_HIGHCOLOR))
                    sprintf(buf, " [1;36m%4d[m %s%c%s %-12.12s %s[1;36m.%c%-47.47s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
                else
                    sprintf(buf, " [36m%4d[m %s%c%s %-12.12s %s[36m.%c%-47.47s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
            else
                sprintf(buf, " %4d %s%c%s %-12.12s %s %c%-47.47s", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
        } else {
            if (strcmp(ReadPost, ent->title) == 0)      /* 当前阅读主题 标识 */
                if (DEFINE(currentuser, DEF_HIGHCOLOR))
                    sprintf(buf, " [1;33m%4d[m %s%c%s %-12.12s %s[1;33m.%c● %-44.44s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
                else
                    sprintf(buf, " [33m%4d[m %s%c%s %-12.12s %s[33m.%c● %-44.44s[m ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
            else
                sprintf(buf, " %4d %s%c%s %-12.12s %s %c● %-44.44s ", num, typeprefix, type, typesufix, ent->owner, date, attachch, TITLE);
        }
    return buf;
}

int add_author_friend(int ent, struct fileheader *fileinfo, char *direct)
{
    if (!strcmp("guest", currentuser->userid))
        return DONOTHING;;

    if (!strcmp(fileinfo->owner, "Anonymous") || !strcmp(fileinfo->owner, "deliver"))
        return DONOTHING;
    else {
        clear();
        addtooverride(fileinfo->owner);
    }
    return FULLUPDATE;
}
extern int zsend_file(char *filename, char *title);
int zsend_post(int ent, struct fileheader *fileinfo, char *direct)
{
    char *t;
    char buf1[512];

    strcpy(buf1, direct);
    if ((t = strrchr(buf1, '/')) != NULL)
        *t = '\0';
    snprintf(genbuf, 512, "%s/%s", buf1, fileinfo->filename);
    return zsend_file(genbuf, fileinfo->title);
}

#define SESSIONLEN 9
void get_telnet_sessionid(char* buf,int unum)
{
    static const char encode[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    struct user_info* pui=get_utmpent(unum);
    int utmpkey=pui->utmpkey;
    buf[0]=encode[unum%36];
    unum/=36;
    buf[1]=encode[unum%36];
    unum/=36;
    buf[2]=encode[unum%36];

    buf[3]=encode[utmpkey%36];
    utmpkey/=36;
    buf[4]=encode[utmpkey%36];
    utmpkey/=36;
    buf[5]=encode[utmpkey%36];
    utmpkey/=36;
    buf[6]=encode[utmpkey%36];
    utmpkey/=36;
    buf[7]=encode[utmpkey%36];
    utmpkey/=36;
    buf[8]=encode[utmpkey%36];
    utmpkey/=36;

    buf[9]=0;
}

void  board_attach_link(char* buf,int buf_len,long attachpos,void* arg)
{
    struct fileheader* fh=(struct fileheader*)arg;
    char* server=sysconf_str("BBS_WEBDOMAIN");
    if (server==NULL)
        server=sysconf_str("BBSDOMAIN");
    snprintf(buf,buf_len-SESSIONLEN,"http://%s/bbscon.php?board=%s&id=%d&ap=%d&sid=",
        server,currboard,fh->id,attachpos);
    get_telnet_sessionid(buf+strlen(buf), utmpent);
}

int zsend_attach(int ent, struct fileheader *fileinfo, char *direct)
{
    char *t;
    char buf1[512];
    char *ptr, *p;
    size_t size;
    long left;

    if(fileinfo->attachment==0) return -1;
    strcpy(buf1, direct);
    if ((t = strrchr(buf1, '/')) != NULL)
        *t = '\0';
    snprintf(genbuf, 512, "%s/%s", buf1, fileinfo->filename);
    BBS_TRY {
        if (safe_mmapfile(genbuf, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, &size, NULL) == 0) {
            BBS_RETURN_VOID;
        }
        for (p=ptr,left=size;left>0;p++,left--) {
            long attach_len;
            char* file,*attach;
            FILE* fp;
            char name[100];
            if (NULL !=(file = checkattach(p, left, &attach_len, &attach))) {
                left-=(attach-p)+attach_len-1;
                p=attach+attach_len-1;
                sprintf(name, "attach%06d", rand()%100000);
                fp=fopen(name, "wb");
                fwrite(attach, 1, attach_len, fp);
                fclose(fp);
                bbs_zsendfile(name, file);
                unlink(name);
                continue;
            }
        }
    }
    BBS_CATCH {
    }
    BBS_END end_mmapfile((void *) ptr, size, -1);
    return 0;
}

int read_post(int ent, struct fileheader *fileinfo, char *direct)
{
    char *t;
    char buf[512];
    int ch;
    int cou;

    /* czz 2003.3.4 forbid reading cancelled post in board */
    if ((fileinfo->owner[0] == '-') && (digestmode != 4) && (digestmode != 5))
	    return FULLUPDATE;

    clear();
    strcpy(buf, direct);
    if ((t = strrchr(buf, '/')) != NULL)
        *t = '\0';
    sprintf(genbuf, "%s/%s", buf, fileinfo->filename);
/*
    strcpy(quote_file, genbuf);
*/
    strcpy(quote_board, currboard);
    strncpy(quote_title, fileinfo->title, 118);
/*    quote_file[119] = fileinfo->filename[STRLEN - 2];
*/
    strncpy(quote_user, fileinfo->owner, OWNER_LEN);
    quote_user[OWNER_LEN - 1] = 0;

    register_attach_link(board_attach_link, fileinfo);
#ifndef NOREPLY
    ch = ansimore_withzmodem(genbuf, false, fileinfo->title);   /* 显示文章内容 */
#else
    ch = ansimore_withzmodem(genbuf, true, fileinfo->title);    /* 显示文章内容 */
#endif
    register_attach_link(NULL,NULL);
    brc_add_read(fileinfo->id);
#ifndef NOREPLY
    move(t_lines - 1, 0);
    if (haspostperm(currentuser, currboard)) {  /* 根据是否有POST权 显示最下一行 */
        if (DEFINE(currentuser, DEF_HIGHCOLOR))
            prints("[44m[1;31m[阅读文章] [33m 回信 R │ 结束 Q,← │上一封 ↑│下一封 <Space>,↓│主题阅读 ^X或p [m");
        else
            prints("[44m[31m[阅读文章] [33m 回信 R │ 结束 Q,← │上一封 ↑│下一封 <Space>,↓│主题阅读 ^X或p [m");
    } else {
        if (DEFINE(currentuser, DEF_HIGHCOLOR))
            prints("[44m[31m[阅读文章]  [33m结束 Q,← │上一封 ↑│下一封 <Space>,<Enter>,↓│主题阅读 ^X 或 p [m");
        else
            prints("[44m[1;31m[阅读文章]  [33m结束 Q,← │上一封 ↑│下一封 <Space>,<Enter>,↓│主题阅读 ^X 或 p [m");
    }

    clrtoeol();                 /* 清屏到行尾 */
    if (!strncmp(fileinfo->title, "Re:", 3)) {
        strncpy(ReplyPost, fileinfo->title,STRLEN);
        for (cou = 0; cou < STRLEN; cou++)
            ReadPost[cou] = ReplyPost[cou + 4];
    } else if (!strncmp(fileinfo->title, "├ ", 3) || !strncmp(fileinfo->title, "└ ", 3)) {
        strcpy(ReplyPost, "Re: ");
        strncat(ReplyPost, fileinfo->title + 3, STRLEN - 4);
        for (cou = 0; cou < STRLEN-4; cou++)
            ReadPost[cou] = ReplyPost[cou + 4];
    } else {
        strcpy(ReplyPost, "Re: ");
        strncat(ReplyPost, fileinfo->title, STRLEN - 4);
        strncpy(ReadPost, fileinfo->title, STRLEN - 1);
        ReadPost[STRLEN - 1] = 0;
    }
    memcpy(&ReadPostHeader, fileinfo, sizeof(struct fileheader));

/* sleep(1); *//*
 * * * * * * * * * * * ????? 
 */
    if (!(ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_PGUP))
        ch = igetkey();

    switch (ch) {
    case Ctrl('Z'):
        r_lastmsg();            /* Leeward 98.07.30 support msgX */
        break;
    case Ctrl('Y'):
        zsend_post(ent, fileinfo, direct);
        break;
    case Ctrl('D'):
        zsend_attach(ent, fileinfo, direct);
        break;
    case 'N':
    case 'Q':
    case 'n':
    case 'q':
    case KEY_LEFT:
        break;
    case KEY_REFRESH:
        break;
    case KEY_RIGHT:
#ifdef NINE_BUILD
        sread(0, 0, ent, 0, fileinfo);
        break;
#endif
    case ' ':
    case 'j':
    case KEY_DOWN:
    case KEY_PGDN:
        return READ_NEXT;
    case KEY_UP:
    case KEY_PGUP:
        return READ_PREV;
    case 'Y':
    case 'R':
    case 'y':
    case 'r':
        if ((!strcmp(currboard, "News")) || (!strcmp(currboard, "Original"))) {
            clear();
            move(3, 0);
            clrtobot();
            prints("\n\n                    很抱歉，该版仅能发表文章,不能回文章...\n");
            pressreturn();
            break;              /*Haohmaru.98.12.19,不能回文章的版 */
        }
        if (fileinfo->accessed[1] & FILE_READ) {        /*Haohmaru.99.01.01.文章不可re */
            clear();
            move(3, 0);
            prints("\n\n            很抱歉，本文已经设置为不可re模式,请不要试图讨论本文...\n");
            pressreturn();
            break;
        }
        do_reply(fileinfo);
        break;
    case Ctrl('R'):
        post_reply(ent, fileinfo, direct);      /* 回文章 */
        break;
    case 'g':
        digest_post(ent, fileinfo, direct);     /* 文摘模式 */
        break;
    case 'M':
        mark_post(ent, fileinfo, direct);       /* Leeward 99.03.02 */
        break;
    case Ctrl('U'):
        sread(0, 1, NULL /*ent */ , 1, fileinfo);       /* Leeward 98.10.03 */
        break;
    case Ctrl('H'):
        sread(-1003, 1, NULL /*ent */ , 1, fileinfo);
        break;
    case Ctrl('N'):
        sread(2, 0, ent, 0, fileinfo);
        sread(3, 0, ent, 0, fileinfo);
        sread(0, 1, ent, 0, fileinfo);
        break;
    case Ctrl('S'):
    case 'p':                  /*Add by SmallPig */
        sread(0, 0, ent, 0, fileinfo);
        break;
    case Ctrl('X'):            /* Leeward 98.10.03 */
        sread(-1003, 0, ent, 0, fileinfo);
        break;
    case Ctrl('Q'):            /*Haohmaru.98.12.05,系统管理员直接查作者资料 */
        clear();
        show_authorinfo(0, fileinfo, '\0');
        return READ_NEXT;
        break;
    case Ctrl('W'):            /*cityhunter 00.10.18察看版主信息 */
        clear();
        show_authorBM(0, fileinfo, '\0');
        return READ_NEXT;
        break;
    case Ctrl('O'):
        clear();
        add_author_friend(0, fileinfo, '\0');
        return READ_NEXT;
    case 'Z':
    case 'z':
        if (!HAS_PERM(currentuser, PERM_PAGE))
            break;
        sendmsgtoauthor(0, fileinfo, '\0');
        return READ_NEXT;
        break;
    case Ctrl('A'):            /*Add by SmallPig */
        clear();
        show_author(0, fileinfo, '\0');
        return READ_NEXT;
        break;
    case 'L':
    case 'l':                  /* Luzi 1997.11.1 */
        if (uinfo.mode != LOOKMSGS) {
            show_allmsgs();
            break;
        } else
            return DONOTHING;
    case '!':                  /*Haohmaru 98.09.24 */
        Goodbye();
        break;
    case 'H':                  /* Luzi 1997.11.1 */
        r_lastmsg();
        break;
    case 'w':                  /* Luzi 1997.11.1 */
        if (!HAS_PERM(currentuser, PERM_PAGE))
            break;
        s_msg();
        break;
    case 'O':
    case 'o':                  /* Luzi 1997.11.1 */
#ifdef NINE_BUILD
    case 'C':
    case 'c':
#endif
        if (!HAS_PERM(currentuser, PERM_BASIC))
            break;
        t_friends();
        break;
    case 'u':                  /* Haohmaru 1999.11.28 */
        clear();
        modify_user_mode(QUERY);
        t_query(NULL);
        break;
    }
#endif
    return FULLUPDATE;
}

int skip_post(int ent, struct fileheader *fileinfo, char *direct)
{
    brc_add_read(fileinfo->id);
    return GOTO_NEXT;
}

int do_select(int ent, struct fileheader *fileinfo, char *direct)
        /*
         * 输入讨论区名 选择讨论区 
         */
{
    char bname[STRLEN], bpath[STRLEN];
    struct stat st;

    move(0, 0);
    prints("选择一个讨论区 (英文字母大小写皆可)");
    clrtoeol();
    prints("\n输入讨论区名 (按空白键自动搜寻): ");
    clrtoeol();

    make_blist();               /* 生成所有Board名 列表 */
    namecomplete((char *) NULL, bname); /* 提示输入 board 名 */
    setbpath(bpath, bname);
    if (*bname == '\0')
    	return FULLUPDATE;
    if (stat(bpath, &st) == -1) { /* 判断board是否存在 */
        move(2, 0);
        prints("不正确的讨论区.\n");
        pressreturn();
        return FULLUPDATE;
    }
    if (!(st.st_mode & S_IFDIR)) {
        move(2, 0);
        prints("不正确的讨论区.\n");
        pressreturn();
        return FULLUPDATE;
    }

    selboard = 1;
    brc_initial(currentuser->userid, bname);

    move(0, 0);
    clrtoeol();
    move(1, 0);
    clrtoeol();
    if (digestmode != false && digestmode != true)
        digestmode = false;
    setbdir(digestmode, direct, currboard);     /* direct 设定 为 当前board目录 */
    return NEWDIRECT;
}

int digest_mode()
{                               /* 文摘模式 切换 */
    if (digestmode == true) {
        digestmode = false;
        setbdir(digestmode, currdirect, currboard);
    } else {
        digestmode = true;
        setbdir(digestmode, currdirect, currboard);
        if (!dashf(currdirect)) {
            digestmode = false;
            setbdir(digestmode, currdirect, currboard);
            return FULLUPDATE;
        }
    }
    return NEWDIRECT;
}

/*stephen : check whether current useris in the list of "jury" 2001.11.1*/
int isJury()
{
    char buf[STRLEN];

    if (!HAS_PERM(currentuser, PERM_JURY))
        return 0;
    makevdir(currboard);
    setvfile(buf, currboard, "jury");
    return seek_in_file(buf, currentuser->userid);
}

int deleted_mode()
{

/* Allow user in file "jury" to see deleted area. stephen 2001.11.1 */
    if (!chk_currBM(currBM, currentuser) && !isJury()) {
        return DONOTHING;
    }
    if (digestmode == 4) {
        digestmode = false;
        setbdir(digestmode, currdirect, currboard);
    } else {
        digestmode = 4;
        setbdir(digestmode, currdirect, currboard);
        if (!dashf(currdirect)) {
            digestmode = false;
            setbdir(digestmode, currdirect, currboard);
            return DONOTHING;
        }
    }
    return NEWDIRECT;
}

int generate_mark()
{
    struct fileheader mkpost;
    struct flock ldata, ldata2;
    int fd, fd2, size = sizeof(fileheader), total, i, count = 0;
    char olddirect[PATHLEN];
    char *ptr, *ptr1;
    struct stat buf;

    digestmode = 0;
    setbdir(digestmode, olddirect, currboard);
    digestmode = 3;
    setbdir(digestmode, currdirect, currboard);
    if ((fd = open(currdirect, O_WRONLY | O_CREAT, 0664)) == -1) {
        bbslog("user", "%s", "recopen err");
        return -1;              /* 创建文件发生错误*/
    }
    ldata.l_type = F_WRLCK;
    ldata.l_whence = 0;
    ldata.l_len = 0;
    ldata.l_start = 0;
    if (fcntl(fd, F_SETLKW, &ldata) == -1) {
        bbslog("user", "%s", "reclock err");
        close(fd);
        return -1;              /* lock error*/
    }
    /* 开始互斥过程*/
    if (!setboardmark(currboard, -1)) {
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return -1;
    }

    if ((fd2 = open(olddirect, O_RDONLY, 0664)) == -1) {
        bbslog("user", "%s", "recopen err");
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return -1;
    }
    fstat(fd2, &buf);
    ldata2.l_type = F_RDLCK;
    ldata2.l_whence = 0;
    ldata2.l_len = 0;
    ldata2.l_start = 0;
    fcntl(fd2, F_SETLKW, &ldata2);
    total = buf.st_size / size;

    BBS_TRY {
        if (safe_mmapfile_handle(fd2, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, (size_t *) & buf.st_size) == 0) {
            ldata2.l_type = F_UNLCK;
            fcntl(fd2, F_SETLKW, &ldata2);
            close(fd2);
            ldata.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &ldata);
            close(fd);
            BBS_RETURN(-1);
        }
        ptr1 = ptr;
        for (i = 0; i < total; i++) {
            memcpy(&mkpost, ptr1, size);
            if (mkpost.accessed[0] & FILE_MARKED) {
                write(fd, &mkpost, size);
                count++;
            }
            ptr1 += size;
        }
    }
    BBS_CATCH {
        ldata2.l_type = F_UNLCK;
        fcntl(fd2, F_SETLKW, &ldata2);
        close(fd2);
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        end_mmapfile((void *) ptr, buf.st_size, -1);
        BBS_RETURN(-1);
    }
    BBS_END end_mmapfile((void *) ptr, buf.st_size, -1);

    ldata2.l_type = F_UNLCK;
    fcntl(fd2, F_SETLKW, &ldata2);
    close(fd2);
    ftruncate(fd, count * size);

    setboardmark(currboard, 0); /* 标记flag*/

    ldata.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &ldata);        /* 退出互斥区域*/
    close(fd);
    return 0;
}

int generate_title()
{
    struct fileheader mkpost, *ptr1, *ptr2;
    struct flock ldata, ldata2;
    int fd, fd2, size = sizeof(fileheader), total, i, j, count = 0, hasht;
    char olddirect[PATHLEN];
    char *ptr, *t;
    struct hashstruct {
        int index, data;
    } *hashtable;
    int *index, *next;
    size_t f_size;

    digestmode = 0;
    setbdir(digestmode, olddirect, currboard);
    digestmode = 2;
    setbdir(digestmode, currdirect, currboard);
    if ((fd = open(currdirect, O_WRONLY | O_CREAT, 0664)) == -1) {
        bbslog("user", "%s", "recopen err");
        return -1;              /* 创建文件发生错误*/
    }
    ldata.l_type = F_WRLCK;
    ldata.l_whence = 0;
    ldata.l_len = 0;
    ldata.l_start = 0;
    if (fcntl(fd, F_SETLKW, &ldata) == -1) {
        bbslog("user", "%s", "reclock err");
        close(fd);
        return -1;              /* lock error*/
    }
    /* 开始互斥过程*/
    if (!setboardtitle(currboard, -1)) {
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return -1;
    }

    if ((fd2 = open(olddirect, O_RDONLY, 0664)) == -1) {
        bbslog("user", "%s", "recopen err");
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return -1;
    }
    ldata2.l_type = F_RDLCK;
    ldata2.l_whence = 0;
    ldata2.l_len = 0;
    ldata2.l_start = 0;
    fcntl(fd2, F_SETLKW, &ldata2);

    index = NULL;
    hashtable = NULL;
    next = NULL;
    BBS_TRY {
        if (safe_mmapfile_handle(fd2, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, &f_size) == 0) {
            ldata2.l_type = F_UNLCK;
            fcntl(fd2, F_SETLKW, &ldata2);
            close(fd2);
            ldata.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &ldata);
            close(fd);
            BBS_RETURN(-1);
        }
        total = f_size / size;
        hasht = total * 8 / 5;
        hashtable = (struct hashstruct *) malloc(sizeof(*hashtable) * hasht);
        if (hashtable == NULL) {
            end_mmapfile((void *) ptr, f_size, -1);
            BBS_RETURN(-1);
        }
        index = (int *) malloc(sizeof(int) * total);
        if (index == NULL) {
            free(hashtable);
            end_mmapfile((void *) ptr, f_size, -1);
            BBS_RETURN(-1);
        }
        next = (int *) malloc(sizeof(int) * total);
        if (next == NULL) {
            free(hashtable);
            free(index);
            end_mmapfile((void *) ptr, f_size, -1);
            BBS_RETURN(-1);
        }
        memset(hashtable, 0xFF, sizeof(*hashtable) * hasht);
        memset(index, 0, sizeof(int) * total);
        ptr1 = (struct fileheader *) ptr;
        for (i = 0; i < total; i++, ptr1++) {
            int l = 0, m;

            if (ptr1->groupid == ptr1->id)
                l = i;
            else {
                l = ptr1->groupid % hasht;
                while (hashtable[l].index != ptr1->groupid && hashtable[l].index != -1) {
                    l++;
                    if (l >= hasht)
                        l = 0;
                }
                if (hashtable[l].index == -1)
                    l = i;
                else
                    l = hashtable[l].data;
            }
            if (l == i) {
                l = ptr1->groupid % hasht;
                while (hashtable[l].index != -1) {
                    l++;
                    if (l >= hasht)
                        l = 0;
                }
                hashtable[l].index = ptr1->groupid;
                hashtable[l].data = i;
                index[i] = i;
                next[i] = 0;
            } else {
                m = index[l];
                next[m] = i;
                next[i] = 0;
                index[l] = i;
                index[i] = -1;
            }
        }
        ptr1 = (struct fileheader *) ptr;
        for (i = 0; i < total; i++, ptr1++)
            if (index[i] != -1) {
                int last;

                write(fd, ptr1, size);
                count++;
                j = next[i];
                while (j != 0) {
                    ptr2 = (struct fileheader *) (ptr + j * size);
                    memcpy(&mkpost, ptr2, sizeof(mkpost));
                    t = ptr2->title;
                    if (!strncmp(t, "Re:", 3))
                        t += 4;
                    if (next[j] == 0)
                        sprintf(mkpost.title, "└ %s", t);
                    else
                        sprintf(mkpost.title, "├ %s", t);
                    write(fd, &mkpost, size);
                    count++;
                    j = next[j];
                }
            }

        free(index);
        free(next);
        free(hashtable);
    }
    BBS_CATCH {
        ldata2.l_type = F_UNLCK;
        fcntl(fd2, F_SETLKW, &ldata2);
        close(fd2);
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        end_mmapfile((void *) ptr, f_size, -1);
        if (index)
            free(index);
        if (next)
            free(next);
        if (hashtable)
            free(hashtable);
        BBS_RETURN(-1);
    }
    BBS_END ldata2.l_type = F_UNLCK;

    fcntl(fd2, F_SETLKW, &ldata2);
    close(fd2);
    ftruncate(fd, count * size);

    setboardtitle(currboard, 0);        /* 标记flag*/

    ldata.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &ldata);        /* 退出互斥区域*/
    close(fd);
    return 0;
}

int marked_mode()
{
    if (digestmode == 3) {
        digestmode = false;
        setbdir(digestmode, currdirect, currboard);
    } else {
        digestmode = 3;
        if (setboardmark(currboard, -1)) {
            if (generate_mark() == -1) {
                digestmode = false;
                return FULLUPDATE;
            }
        }
        setbdir(digestmode, currdirect, currboard);
        if (!dashf(currdirect)) {
            digestmode = false;
            setbdir(digestmode, currdirect, currboard);
            return FULLUPDATE;
        }
    }
    return NEWDIRECT;
}

int title_mode()
{
    struct stat st;

    if (!stat("heavyload", &st)) {
        move(t_lines - 1, 0);
        clrtoeol();
        prints("系统负担过重，暂时不能响应主题阅读的请求...");
        pressanykey();
        return FULLUPDATE;
    }

    digestmode = 2;
    if (setboardtitle(currboard, -1)) {
        if (generate_title() == -1) {
            digestmode = false;
            return FULLUPDATE;
        }
    }
    setbdir(digestmode, currdirect, currboard);
    if (!dashf(currdirect)) {
        digestmode = false;
        setbdir(digestmode, currdirect, currboard);
        return FULLUPDATE;
    }
    return NEWDIRECT;
}

static char search_data[STRLEN];
int search_mode(int mode, char *index)
/* added by bad 2002.8.8 search mode*/
{
    struct fileheader *ptr1;
    struct flock ldata, ldata2;
    int fd, fd2, size = sizeof(fileheader), total, i, count = 0;
    char olddirect[PATHLEN];
    char *ptr;
    struct stat buf;
    bool init;
    size_t bm_search[256];

    strncpy(search_data, index, STRLEN);
    digestmode = 0;
    setbdir(digestmode, olddirect, currboard);
    digestmode = mode;
    setbdir(digestmode, currdirect, currboard);
    if (mode == 6 && !setboardorigin(currboard, -1)) {
        return NEWDIRECT;
    }
    if ((fd = open(currdirect, O_WRONLY | O_CREAT, 0664)) == -1) {
        bbslog("user", "%s", "recopen err");
        return FULLUPDATE;      /* 创建文件发生错误*/
    }
    ldata.l_type = F_WRLCK;
    ldata.l_whence = 0;
    ldata.l_len = 0;
    ldata.l_start = 0;
    if (fcntl(fd, F_SETLKW, &ldata) == -1) {
        bbslog("user", "%s", "reclock err");
        close(fd);
        return FULLUPDATE;      /* lock error*/
    }
    /* 开始互斥过程*/
    if (mode == 6 && !setboardorigin(currboard, -1)) {
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return -1;
    }

    if ((fd2 = open(olddirect, O_RDONLY, 0664)) == -1) {
        bbslog("user", "%s", "recopen err");
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return FULLUPDATE;
    }
    fstat(fd2, &buf);
    ldata2.l_type = F_RDLCK;
    ldata2.l_whence = 0;
    ldata2.l_len = 0;
    ldata2.l_start = 0;
    fcntl(fd2, F_SETLKW, &ldata2);
    total = buf.st_size / size;

    init = false;
    if ((i = safe_mmapfile_handle(fd2, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &ptr, &buf.st_size)) != 1) {
        if (i == 2)
            end_mmapfile((void *) ptr, buf.st_size, -1);
        ldata2.l_type = F_UNLCK;
        fcntl(fd2, F_SETLKW, &ldata2);
        close(fd2);
        ldata.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &ldata);
        close(fd);
        return FULLUPDATE;
    }
    ptr1 = (struct fileheader *) ptr;
    for (i = 0; i < total; i++) {
        if (mode == 6 && ptr1->id == ptr1->groupid || mode == 7 && strcasecmp(ptr1->owner, index) == 0 || mode == 8 && bm_strcasestr_rp(ptr1->title, index, bm_search, &init) != NULL) {
            write(fd, ptr1, size);
            count++;
        }
        ptr1++;
    }
    end_mmapfile((void *) ptr, buf.st_size, -1);
    ldata2.l_type = F_UNLCK;
    fcntl(fd2, F_SETLKW, &ldata2);
    close(fd2);
    ftruncate(fd, count * size);

    if (mode == 6)
        setboardorigin(currboard, 0);   /* 标记flag*/

    ldata.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &ldata);        /* 退出互斥区域*/
    close(fd);
    return NEWDIRECT;
}

int change_mode(int ent, struct fileheader *fileinfo, char *direct)
{
    char ans[4];
    char buf[STRLEN], buf2[STRLEN];
    static char title[31] = "";

    if (digestmode > 0) {
        if (digestmode == 7 || digestmode == 8)
            unlink(currdirect);
        digestmode = 0;
        setbdir(digestmode, currdirect, currboard);
        return NEWDIRECT;
    }
    move(t_lines - 1, 0);
    clrtoeol();
    getdata(t_lines - 1, 0, "切换模式到: 1)文摘 2)同主题 3)被m文章 4)原作 5)同作者 6)标题关键字 [1]: ", ans, 3, DOECHO, NULL, true);
    if (ans[0] == ' ') {
        ans[0] = ans[1];
        ans[1] = 0;
    }
    if (ans[0] == '5') {
        move(t_lines - 1, 0);
        clrtoeol();
        sprintf(buf, "您想查找哪位网友的文章[%s]: ", fileinfo->owner);
        getdata(t_lines - 1, 0, buf, buf2, 13, DOECHO, NULL, true);
        if (buf2[0])
            strcpy(buf, buf2);
        else
            strcpy(buf, fileinfo->owner);
        if (buf[0] == 0)
            return FULLUPDATE;
    } else if (ans[0] == '6') {
        move(t_lines - 1, 0);
        clrtoeol();
        sprintf(buf, "您想查找的文章标题关键字[%s]: ", title);
        getdata(t_lines - 1, 0, buf, buf2, 30, DOECHO, NULL, true);
        if (buf2[0])
            strcpy(title, buf2);
        strcpy(buf, title);
        if (buf[0] == 0)
            return FULLUPDATE;
    }
    switch (ans[0]) {
    case 0:
    case '1':
        return digest_mode();
        break;
    case '2':
        return title_mode();
        break;
    case '3':
        return marked_mode();
        break;
    case '4':
        return search_mode(6, buf);
        break;
    case '5':
        return search_mode(7, buf);
        break;
    case '6':
        return search_mode(8, buf);
        break;
    }
    return FULLUPDATE;
}

int read_hot_info(int ent, struct fileheader *fileinfo, char *direct)
{
    char ans[4];
    move(t_lines - 1, 0);
    clrtoeol();
    getdata(t_lines - 1, 0, "选择阅读: 1)本日十大新闻 2)本日十大祝福 3)近期热点 [1]: ", ans, 3, DOECHO, NULL, true);
    switch (ans[0])
	{
    case '2':
        show_help("etc/posts/bless");
        break;
    case '3':
		show_help("0Announce/hotinfo");
        break;
    case '1':
	default:
		show_help("etc/posts/day");
    }
    return FULLUPDATE;
}

int junk_mode()
{
    if (!HAS_PERM(currentuser, PERM_SYSOP)) {
        return DONOTHING;
    }

    if (digestmode == 5) {
        digestmode = false;
        setbdir(digestmode, currdirect, currboard);
    } else {
        digestmode = 5;
        setbdir(digestmode, currdirect, currboard);
        if (!dashf(currdirect)) {
            digestmode = false;
            setbdir(digestmode, currdirect, currboard);
            return DONOTHING;
        }
    }
    return NEWDIRECT;
}

int do_thread()
{
    char buf[STRLEN];

    sprintf(buf, "Threading %s", currboard);
    bbslog("user", "%s", buf);
    move(t_lines - 1, 0);
    clrtoeol();
    prints("[5m请稍候，系统处理标题中...[m\n");
    /*
     * sprintf(buf,"bin/thread %s&",currboard); 
     */
    sprintf(buf, "bin/thread %s", currboard);
    system(buf);
    return 0;
}

int thread_mode()
{
    struct stat st;

    /*
     * add by Bigman to verrify the thread 
     */
    char buf[STRLEN], ch[4];

    /*
     * add by KCN if  heavyload can't thread 
     */
    if (!stat("heavyload", &st)) {
        move(t_lines - 1, 0);
        clrtoeol();
        prints("系统负担过重，暂时不能响应主题阅读的请求...");
        pressanykey();
        return FULLUPDATE;
    }

    if (digestmode == 2) {
        digestmode = false;
        setbdir(digestmode, currdirect, currboard);
    } else {
        /*
         * 增加判断 Bigman 2000.12.26 
         */
        saveline(t_lines - 2, 0, NULL);
        move(t_lines - 2, 0);
        clrtoeol();
        strcpy(buf, "确定同主题阅读? (Y/N) [N]:");
        getdata(t_lines - 2, 0, buf, ch, 3, DOECHO, NULL, true);

        if (ch[0] == 'y' || ch[0] == 'Y') {
            digestmode = 2;
            setbdir(digestmode, currdirect, currboard);
            do_thread();
            /*
             * while(!dashf(currdirect))
             * {
             * sleep(1);
             * if(++i==5)
             * break;
             * } 
             */
            if (!dashf(currdirect)) {
                digestmode = false;
                setbdir(digestmode, currdirect, currboard);
                return PARTUPDATE;
            }
        } else {
            saveline(t_lines - 2, 1, NULL);
            return DONOTHING;
        }
    }
    return NEWDIRECT;
}

int digest_post(int ent, struct fileheader *fhdr, char *direct)
{
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fhdr, direct, FILE_DIGEST_FLAG, 1);
}

#ifndef NOREPLY
int do_reply(struct fileheader *fileinfo)
/* reply POST */
{
    char buf[255];

    if (fileinfo->accessed[1] & FILE_READ) {    /*Haohmaru.99.01.01.文章不可re */
        clear();
        move(3, 0);
        prints("\n\n            很抱歉，本文已经设置为不可re模式,请不要试图讨论本文...\n");
        pressreturn();
        return FULLUPDATE;
    }
    setbfile(buf, currboard, fileinfo->filename);
    strcpy(replytitle, fileinfo->title);
    post_article(buf, fileinfo);
    replytitle[0] = '\0';
    return FULLUPDATE;
}
#endif

int garbage_line(char *str)
{                               /* 判断本行是否是 无用的 */
    int qlevel = 0;

#ifdef NINE_BUILD
#define QUOTELEV 1
#else
#define QUOTELEV 0
#endif
    while (*str == ':' || *str == '>') {
        str++;
        if (*str == ' ')
            str++;
        if (qlevel++ >= QUOTELEV)
            return 1;
    }
    while (*str == ' ' || *str == '\t')
        str++;
    if (qlevel >= QUOTELEV)
        if (strstr(str, "提到:\n") || strstr(str, ": 】\n") || strncmp(str, "==>", 3) == 0 || strstr(str, "的文章 说"))
            return 1;
    return (*str == '\n');
}

/* When there is an old article that can be included -jjyang */
#ifdef NINE_BUILD
void do_quote(char *filepath, char quote_mode, char *q_file, char *q_user)
{
    FILE *inf, *outf;
    char *qfile, *quser;
    char buf[256], *ptr;
    char op;
    int bflag;

    qfile = q_file;
    quser = q_user;
    bflag = strncmp(qfile, "mail", 4);
    outf = fopen(filepath, "w");
    if (*qfile != '\0' && (inf = fopen(qfile, "r")) != NULL) {
        op = quote_mode;
        if (op != 'N') {
            fgets(buf, 256, inf);
            if ((ptr = strrchr(buf, ')')) != NULL) {
                ptr[1] = '\0';
                if ((ptr = strchr(buf, ':')) != NULL) {
                    quser = ptr + 1;
                    while (*quser == ' ')
                        quser++;
                }
            }

            if (bflag)
                fprintf(outf, "【 在 %-.55s 的大作中提到: 】\n", quser);
            else
                fprintf(outf, "【 在 %-.55s 的来信中提到: 】\n", quser);

            if (op == 'A') {
                while (fgets(buf, 256, inf) != NULL) {
                    fprintf(outf, ": %s", buf);
                }
            } else if (op == 'R') {
                while (fgets(buf, 256, inf) != NULL)
                    if (buf[0] == '\n')
                        break;
                while (fgets(buf, 256, inf) != NULL) {
                    if (Origin2(buf))
                        continue;
                    fprintf(outf, "%s", buf);
                }
            } else {
                while (fgets(buf, 256, inf) != NULL)
                    if (buf[0] == '\n')
                        break;
                while (fgets(buf, 256, inf) != NULL) {
                    if (strcmp(buf, "--\n") == 0)
                        break;
                    if (buf[250] != '\0')
                        strcpy(buf + 250, "\n");
                    if (!garbage_line(buf))
                        fprintf(outf, ": %s", buf);
                }
            }
        }
        fprintf(outf, "\n");
        fclose(inf);
    }

    if ((numofsig > 0) && !(currentuser->signature == 0 || Anony == 1)) {       /* 签名档为0则不添加 */
        if (currentuser->signature < 0)
            addsignature(outf, currentuser, (rand() % numofsig) + 1);
        else
            addsignature(outf, currentuser, currentuser->signature);
    }
    fclose(outf);
}

#else
void do_quote(char *filepath, char quote_mode, char *q_file, char *q_user)
{                               /* 引用文章， 全局变量quote_file,quote_user, */
    FILE *inf, *outf;
    char *qfile, *quser;
    char buf[256], *ptr;
    char op;
    int bflag;
    int line_count = 0;         /* 添加简略模式计数 Bigman: 2000.7.2 */

    qfile = q_file;
    quser = q_user;
    bflag = strncmp(qfile, "mail", 4);  /* 判断引用的是文章还是信 */
    outf = fopen(filepath, "w");
    if (outf==NULL) {
    	bbslog("3user","do_quote() fopen(%s):%s",filepath,strerror(errno));
    	return;
    }
    if (*qfile != '\0' && (inf = fopen(qfile, "r")) != NULL) {  /* 打开被引用文件 */
        op = quote_mode;
        if (op != 'N') {        /* 引用模式为 N 表示 不引用 */
            fgets(buf, 256, inf);       /* 取出第一行中 被引用文章的 作者信息 */
            if ((ptr = strrchr(buf, ')')) != NULL) {    /* 第一个':'到最后一个 ')' 中的字符串 */
                ptr[1] = '\0';
                if ((ptr = strchr(buf, ':')) != NULL) {
                    quser = ptr + 1;
                    while (*quser == ' ')
                        quser++;
                }
            }
            /*---	period	2000-10-21	add '\n' at beginning of Re-article	---*/
            if (bflag)
                fprintf(outf, "\n【 在 %s 的大作中提到: 】\n", quser);
            else
                fprintf(outf, "\n【 在 %s 的来信中提到: 】\n", quser);

            if (op == 'A') {    /* 除第一行外，全部引用 */
                while (fgets(buf, 256, inf) != NULL) {
                    fprintf(outf, ": %s", buf);
                }
            } else if (op == 'R') {
                while (fgets(buf, 256, inf) != NULL)
                    if (buf[0] == '\n')
                        break;
                while (fgets(buf, 256, inf) != NULL) {
                    if (Origin2(buf))   /* 判断是否 多次引用 */
                        continue;
                    fprintf(outf, "%s", buf);

                }
            } else {
                while (fgets(buf, 256, inf) != NULL)
                    if (buf[0] == '\n')
                        break;
                while (fgets(buf, 256, inf) != NULL) {
                    if (strcmp(buf, "--\n") == 0)       /* 引用 到签名档为止 */
                        break;
                    if (buf[250] != '\0')
                        strcpy(buf + 250, "\n");
                    if (!garbage_line(buf)) {   /* 判断是否是无用行 */
                        fprintf(outf, ": %s", buf);
                        if (op == 'S') {        /* 简略模式,只引用前几行 Bigman:2000.7.2 */
                            line_count++;
                            if (line_count > 10) {
                                fprintf(outf, ": ...................");
                                break;
                            }
                        }
                    }
                }
            }
        }

        fprintf(outf, "\n");
        fclose(inf);
    }
    /*
     * *q_file = '\0';
     * *q_user = '\0';
     */

    if ((numofsig > 0) && !(currentuser->signature == 0 || Anony == 1)) {       /* 签名档为0则不添加 */
        if (currentuser->signature < 0)
            addsignature(outf, currentuser, (rand() % numofsig) + 1);
        else
            addsignature(outf, currentuser, currentuser->signature);
    }
    fclose(outf);
}
#endif
int do_post()
{                               /* 用户post */
    *quote_user = '\0';
    return post_article("", NULL);
}

 /*ARGSUSED*/ int post_reply(int ent, struct fileheader *fileinfo, char *direct)
        /*
         * 回信给POST作者 
         */
{
    char uid[STRLEN];
    char title[STRLEN];
    char *t;
    FILE *fp;
    char q_file[STRLEN];


    if (!HAS_PERM(currentuser, PERM_LOGINOK) || !strcmp(currentuser->userid, "guest"))  /* guest 无权 */
        return 0;
    /*
     * 太狠了吧,被封post就不让回信了
     * if (!HAS_PERM(currentuser,PERM_POST)) return; Haohmaru.99.1.18 
     */

    /*
     * 封禁Mail Bigman:2000.8.22 
     */
    if (HAS_PERM(currentuser, PERM_DENYMAIL)) {
        clear();
        move(3, 10);
        prints("很抱歉,您目前没有Mail权限!");
        pressreturn();
        return FULLUPDATE;
    }

    modify_user_mode(SMAIL);

    /*
     * indicate the quote file/user 
     */
    setbfile(q_file, currboard, fileinfo->filename);
    strncpy(quote_user, fileinfo->owner, OWNER_LEN);
    quote_user[OWNER_LEN - 1] = 0;

    /*
     * find the author 
     */
    if (strchr(quote_user, '.')) {
        genbuf[0] = '\0';
        fp = fopen(q_file, "r");
        if (fp != NULL) {
            fgets(genbuf, 255, fp);
            fclose(fp);
        }

        t = strtok(genbuf, ":");
        if (strncmp(t, "发信人", 6) == 0 || strncmp(t, "Posted By", 9) == 0 || strncmp(t, "作  家", 6) == 0) {
            t = (char *) strtok(NULL, " \r\t\n");
            strcpy(uid, t);
        } else {
            prints("Error: Cannot find Author ... \n");
            pressreturn();
        }
    } else
        strcpy(uid, quote_user);

    /*
     * make the title 
     */
    if (toupper(fileinfo->title[0]) != 'R' || fileinfo->title[1] != 'e' || fileinfo->title[2] != ':')
        strcpy(title, "Re: ");
    else
        title[0] = '\0';
    strncat(title, fileinfo->title, STRLEN - 5);

    clear();

    /*
     * edit, then send the mail 
     */
    switch (do_send(uid, title, q_file)) {
    case -1:
        prints("系统无法送信\n");
        break;
    case -2:
        prints("送信动作已经中止\n");
        break;
    case -3:
        prints("使用者 '%s' 无法收信\n", uid);
        break;
    case -4:
        prints("对方信箱已满，无法收信\n");
        break;
    default:
        prints("信件已成功地寄给原作者 %s\n", uid);
    }
    pressreturn();
    return FULLUPDATE;
}

int show_board_notes(char bname[30])
{                               /* 显示版主的话 */
    char buf[256];

    sprintf(buf, "vote/%s/notes", bname);       /* 显示本版的版主的话 vote/版名/notes */
    if (dashf(buf)) {
        ansimore2(buf, false, 0, 23 /*19 */ );
        return 1;
    } else if (dashf("vote/notes")) {   /* 显示系统的话 vote/notes */
        ansimore2("vote/notes", false, 0, 23 /*19 */ );
        return 1;
    }
    return -1;
}

int post_article(char *q_file, struct fileheader *re_file)
{                               /*用户 POST 文章 */
    struct fileheader post_file;
    char filepath[STRLEN];
    char buf[256], buf2[256], buf3[STRLEN], buf4[STRLEN];
    int aborted, anonyboard, olddigestmode = 0;
    int replymode = 1;          /* Post New UI */
    char ans[4], include_mode = 'S';
    struct boardheader *bp;
    long eff_size;/*用于统计文章的有效字数*/

#ifdef FILTER
    int returnvalue;
#endif

    if (true == check_readonly(currboard))      /* Leeward 98.03.28 */
        return FULLUPDATE;

#ifdef AIX_CANCELLED_BY_LEEWARD
    if (true == check_RAM_lack())       /* Leeward 98.06.16 */
        return FULLUPDATE;
#endif

    modify_user_mode(POSTING);
    if (digestmode == 2 || digestmode == 3) {
        olddigestmode = digestmode;
        digestmode = 0;
        setbdir(digestmode, currdirect, currboard);
    }
    if (!haspostperm(currentuser, currboard)) { /* POST权限检查 */
        move(3, 0);
        clrtobot();
        if (digestmode == false) {
            prints("\n\n        此讨论区是唯读的, 或是您尚无权限在此发表文章.\n");
            prints("        如果您尚未注册，请在个人工具箱内详细注册身份\n");
            prints("        未通过身份注册认证的用户，没有发表文章的权限。\n");
            prints("        谢谢合作！ :-) \n");
        } else {
            prints("\n\n     目前是文摘或主题模式, 所以不能发表文章.(按左键离开文摘模式)\n");
        }
        pressreturn();
        clear();
        return FULLUPDATE;
    } else if (deny_me(currentuser->userid, currboard) && !HAS_PERM(currentuser, PERM_SYSOP)) { /* 版主禁止POST 检查 */
        move(3, 0);
        clrtobot();
        prints("\n\n                     很抱歉，你被版主停止了 POST 的权力...\n");
        pressreturn();
        clear();
        return FULLUPDATE;
    }

    memset(&post_file, 0, sizeof(post_file));
    clear();
    show_board_notes(currboard);        /* 版主的话 */
#ifndef NOREPLY                 /* title是否不用Re: */
    if (replytitle[0] != '\0') {
        buf4[0] = ' ';
        /*
         * if( strncasecmp( replytitle, "Re:", 3 ) == 0 ) Change By KCN:
         * why use strncasecmp? 
         */
        if (strncmp(replytitle, "Re:", 3) == 0)
            strcpy(buf, replytitle);
        else if (strncmp(replytitle, "├ ", 3) == 0)
            sprintf(buf, "Re: %s", replytitle + 3);
        else if (strncmp(replytitle, "└ ", 3) == 0)
            sprintf(buf, "Re: %s", replytitle + 3);
        else
            sprintf(buf, "Re: %s", replytitle);
        buf[50] = '\0';
    } else
#endif
    {
        buf[0] = '\0';
        buf4[0] = '\0';
        replymode = 0;
    }
    if (currentuser->signature > numofsig)      /*签名档No.检查 */
        currentuser->signature = 1;
    anonyboard = anonymousboard(currboard);     /* 是否为匿名版 */
    if (!strcmp(currboard, "Announce"))
        Anony = 1;
    else if (anonyboard)
        Anony = ANONYMOUS_DEFAULT;
    else
        Anony = 0;

    while (1) {                 /* 发表前修改参数， 可以考虑添加'显示签名档' */
        sprintf(buf3, "引言模式 [%c]", include_mode);
        move(t_lines - 4, 0);
        clrtoeol();
        prints("[m发表文章于 %s 讨论区     %s\n", currboard, (anonyboard) ? (Anony == 1 ? "[1m要[m使用匿名" : "[1m不[m使用匿名") : "");
        clrtoeol();
        prints("使用标题: %-50s\n", (buf[0] == '\0') ? "[正在设定主题]" : buf);
        clrtoeol();
        if (currentuser->signature < 0)
            prints("使用随机签名档     %s", (replymode) ? buf3 : " ");
        else
            prints("使用第 %d 个签名档     %s", currentuser->signature, (replymode) ? buf3 : " ");

        if (buf4[0] == '\0' || buf4[0] == '\n') {
            move(t_lines - 1, 0);
            clrtoeol();
            strcpy(buf4, buf);
            getdata(t_lines - 1, 0, "标题: ", buf4, 50, DOECHO, NULL, false);
            if ((buf4[0] == '\0' || buf4[0] == '\n')) {
                if (buf[0] != '\0') {
                    buf4[0] = ' ';
                    continue;
                } else
                    return FULLUPDATE;
            }
            strcpy(buf, buf4);
            continue;
        }
        move(t_lines - 1, 0);
        clrtoeol();
        /*
         * Leeward 98.09.24 add: viewing signature(s) while setting post head 
         */
        sprintf(buf2, "按[1;32m0[m~[1;32m%d/V/L[m选/看/随机签名档%s，[1;32mT[m改标题，%s[1;32mEnter[m接受所有设定: ", numofsig,
                (replymode) ? "，[1;32mS/Y[m/[1;32mN[m/[1;32mR[m/[1;32mA[m 改引言模式" : "", (anonyboard) ? "[1;32mM[m匿名，" : "");
        if(replymode&&anonyboard) buf2[strlen(buf2)-10]=0;
        getdata(t_lines - 1, 0, buf2, ans, 3, DOECHO, NULL, true);
        ans[0] = toupper(ans[0]);       /* Leeward 98.09.24 add; delete below toupper */
        if ((ans[0] - '0') >= 0 && ans[0] - '0' <= 9) {
            if (atoi(ans) <= numofsig)
                currentuser->signature = atoi(ans);
        } else if ((ans[0] == 'S' || ans[0] == 'Y' || ans[0] == 'N' || ans[0] == 'A' || ans[0] == 'R') && replymode) {
            include_mode = ans[0];
        } else if (ans[0] == 'T') {
            buf4[0] = '\0';
        } else if (ans[0] == 'M') {
            Anony = (Anony == 1) ? 0 : 1;
        } else if (ans[0] == 'L') {
            currentuser->signature = -1;
        } else if (ans[0] == 'V') {     /* Leeward 98.09.24 add: viewing signature(s) while setting post head */
            sethomefile(buf2, currentuser->userid, "signatures");
            move(t_lines - 1, 0);
            if (askyn("预设显示前三个签名档, 要显示全部吗", false) == true)
                ansimore(buf2, 0);
            else {
                clear();
                ansimore2(buf2, false, 0, 18);
            }
        } else {
            /*
             * Changed by KCN,disable color title 
             */
            {
                unsigned int i;

                for (i = 0; (i < strlen(buf)) && (i < STRLEN - 1); i++)
                    if (buf[i] == 0x1b)
                        post_file.title[i] = ' ';
                    else
                        post_file.title[i] = buf[i];
                post_file.title[i] = 0;
            }
            /*
             * strcpy(post_file.title, buf); 
             */
            strncpy(save_title, post_file.title, STRLEN);
            if (save_title[0] == '\0') {
                if (olddigestmode) {
                    digestmode = olddigestmode;
                    setbdir(digestmode, currdirect, currboard);
                }
                return FULLUPDATE;
            }
            break;
        }
    }                           /* 输入结束 */

    setbfile(filepath, currboard, "");
    if ((aborted = GET_POSTFILENAME(post_file.filename, filepath)) != 0) {
        move(3, 0);
        clrtobot();
        prints("\n\n无法创建文件:%d...\n", aborted);
        pressreturn();
        clear();
        if (olddigestmode) {
            digestmode = olddigestmode;
            setbdir(digestmode, currdirect, currboard);
        }
        return FULLUPDATE;
    }

    in_mail = false;

    /*
     * strncpy(post_file.owner,(anonyboard&&Anony)?
     * "Anonymous":currentuser->userid,STRLEN) ;
     */
    strncpy(post_file.owner, (anonyboard && Anony) ? currboard : currentuser->userid, OWNER_LEN);
    post_file.owner[OWNER_LEN - 1] = 0;

    /*
     * if ((!strcmp(currboard,"Announce"))&&(!strcmp(post_file.owner,"Anonymous")))
     * strcpy(post_file.owner,"SYSOP");
     */

    if ((!strcmp(currboard, "Announce")) && (!strcmp(post_file.owner, currboard)))
        strcpy(post_file.owner, "SYSOP");

    setbfile(filepath, currboard, post_file.filename);

    if ((bp = getbcache(currboard)) == NULL) {
        unlink(filepath);
        clear();
        if (olddigestmode) {
            digestmode = olddigestmode;
            setbdir(digestmode, currdirect, currboard);
        }
        return FULLUPDATE;
    }
    if (bp->flag & BOARD_OUTFLAG)
        local_article = 0;
    else
        local_article = 1;
    if (!strcmp(post_file.title, buf) && q_file[0] != '\0')
        if (q_file[119] == 'L') /* FIXME */
            local_article = 1;

    modify_user_mode(POSTING);

    do_quote(filepath, include_mode, q_file, quote_user);       /*引用原文章 */

    strcpy(quote_title, save_title);
    strcpy(quote_board, currboard);
    aborted = vedit(filepath, true, &eff_size, NULL);    /* 进入编辑状态 */

    add_loginfo(filepath, currentuser, currboard, Anony);       /*添加最后一行 */

    strncpy(post_file.title, save_title, STRLEN);
    if (aborted == 1 || !(bp->flag & BOARD_OUTFLAG)) {  /* local save */
        post_file.innflag[1] = 'L';
        post_file.innflag[0] = 'L';
    } else {
        post_file.innflag[1] = 'S';
        post_file.innflag[0] = 'S';
        outgo_post(&post_file, currboard, save_title);
    }
    Anony = 0;                  /*Inital For ShowOut Signature */

    if (aborted == -1) {        /* 取消POST */
        unlink(filepath);
        clear();
        if (olddigestmode) {
            digestmode = olddigestmode;
            setbdir(digestmode, currdirect, currboard);
        }
        return FULLUPDATE;
    }
    setbdir(digestmode, buf, currboard);

    /*
     * 在boards版版主发文自动添加文章标记 Bigman:2000.8.12 
     */
    if (!strcmp(currboard, "Board") && !HAS_PERM(currentuser, PERM_OBOARDS) && HAS_PERM(currentuser, PERM_BOARDS)) {
        post_file.accessed[0] |= FILE_SIGN;
    }
#ifdef FILTER
    returnvalue =
#endif
        after_post(currentuser, &post_file, currboard, re_file);

    if (!junkboard(currboard)) {
        currentuser->numposts++;
    }
#ifdef FILTER
    if (returnvalue == 2) {
        clear();
        move(3, 0);
        prints("\n\n            很抱歉，本文可能含有不适当的内容，需经审核方可发\n表，请耐心等待...\n");
        pressreturn();
    }
#endif
    switch (olddigestmode) {
    case 2:
        title_mode();
        break;
    case 3:
        marked_mode();
        break;
    }
    return FULLUPDATE;
}

 /*ARGSUSED*/ int edit_post(int ent, struct fileheader *fileinfo, char *direct)
        /*
         * POST 编辑
         */
{
    char buf[512];
    char *t;
    long eff_size;
    long attachpos;

    if (!strcmp(currboard, "syssecurity")
        || !strcmp(currboard, "junk")
        || !strcmp(currboard, "deleted"))       /* Leeward : 98.01.22 */
        return DONOTHING;
    if ((digestmode == 4) || (digestmode == 5))
        return DONOTHING;       /* no edit in dustbin as requested by terre */
    if (true == check_readonly(currboard))      /* Leeward 98.03.28 */
        return FULLUPDATE;

#ifdef AIX_CANCELLED_BY_LEEWARD
    if (true == check_RAM_lack())       /* Leeward 98.06.16 */
        return FULLUPDATE;
#endif

    modify_user_mode(EDIT);

    if (!HAS_PERM(currentuser, PERM_SYSOP))     /* SYSOP、当前版主、原发信人 可以编辑 */
        if (!chk_currBM(currBM, currentuser))
            /*
             * change by KCN 1999.10.26
             * if(strcmp( fileinfo->owner, currentuser->userid))
             */
            if (!isowner(currentuser, fileinfo))
                return DONOTHING;

    if (deny_me(currentuser->userid, currboard) && !HAS_PERM(currentuser, PERM_SYSOP)) {        /* 版主禁止POST 检查 */
        move(3, 0);
        clrtobot();
        prints("\n\n                     很抱歉，你被版主停止了 POST 的权力...\n");
        pressreturn();
        clear();
        return FULLUPDATE;
    }

    clear();
    strcpy(buf, direct);
    if ((t = strrchr(buf, '/')) != NULL)
        *t = '\0';
#ifndef LEEWARD_X_FILTER
    sprintf(genbuf, "/bin/cp -f %s/%s tmp/%d.editpost.bak", buf, fileinfo->filename, getpid()); /* Leeward 98.03.29 */
    system(genbuf);
#endif

    /*
     * Leeward 2000.01.23: Cache 
     * sprintf(genbuf, "/board/%s/%s.html", currboard,fileinfo->filename);
     * ca_expire(genbuf); 
     */

    sprintf(genbuf, "%s/%s", buf, fileinfo->filename);
    if (vedit_post(genbuf, false, &eff_size,&attachpos) != -1) {
        if (ADD_EDITMARK)
            add_edit_mark(genbuf, 0, /*NULL*/ fileinfo->title);
        if (attachpos!=fileinfo->attachment) {
            fileinfo->attachment=attachpos;
            change_post_flag(currBM, currentuser, digestmode, currboard, ent, 
                fileinfo, direct, FILE_ATTACHPOS_FLAG, 0);
        }
    }
    newbbslog(BBSLOG_USER, "edited post '%s' on %s", fileinfo->title, currboard);
    return FULLUPDATE;
}

int edit_title(int ent, struct fileheader *fileinfo, char *direct)
        /*
         * 编辑文章标题 
         */
{
    char buf[STRLEN];

    /*
     * Leeward 99.07.12 added below 2 variables 
     */
    long i;
    struct fileheader xfh;
    int fd;


    if (!strcmp(currboard, "syssecurity")
        || !strcmp(currboard, "junk")
        || !strcmp(currboard, "deleted"))       /* Leeward : 98.01.22 */
        return DONOTHING;

    if (digestmode >= 2)
        return DONOTHING;
    if (true == check_readonly(currboard))      /* Leeward 98.03.28 */
        return FULLUPDATE;

    if (!HAS_PERM(currentuser, PERM_SYSOP))     /* 权限检查 */
        if (!chk_currBM(currBM, currentuser))
            /*
             * change by KCN 1999.10.26
             * if(strcmp( fileinfo->owner, currentuser->userid))
             */
            if (!isowner(currentuser, fileinfo)) {
                return DONOTHING;
            }
    strcpy(buf, fileinfo->title);
    getdata(t_lines - 1, 0, "新文章标题: ", buf, 50, DOECHO, NULL, false);      /*输入标题 */
    if (buf[0] != '\0') {
        char tmp[STRLEN * 2], *t;
        char tmp2[STRLEN];      /* Leeward 98.03.29 */

        /*
         * Leeward 2000.01.23: Cache 
         * sprintf(genbuf, "/board/%s/%s.html", currboard,fileinfo->filename);
         * ca_expire(genbuf); 
         */

#ifdef FILTER
        if (check_badword_str(buf, strlen(buf))) {
            clear();
            move(3, 0);
            outs("     很抱歉，该标题可能含有不恰当的内容，请仔细检查换个标题。");
            pressreturn();
            return PARTUPDATE;
        }
#endif
        strcpy(tmp2, fileinfo->title);  /* Do a backup */
        /*
         * Changed by KCN,disable color title
         */
        {
            unsigned int i;

            for (i = 0; (i < strlen(buf)) && (i < STRLEN - 1); i++)
                if (buf[i] == 0x1b)
                    fileinfo->title[i] = ' ';
                else
                    fileinfo->title[i] = buf[i];
            fileinfo->title[i] = 0;
        }
        /*
         * strcpy(fileinfo->title,buf);
         */
        strcpy(tmp, direct);
        if ((t = strrchr(tmp, '/')) != NULL)
            *t = '\0';
        sprintf(genbuf, "%s/%s", tmp, fileinfo->filename);

        if(strcmp(tmp2,buf)) add_edit_mark(genbuf, 2, buf);
        /*
         * Leeward 99.07.12 added below to fix a big bug
         */
	/* modified by stiger */
        //setbdir(digestmode, buf, currboard);
        //if ((fd = open(buf, O_RDONLY, 0)) != -1) {
		/* add by stiger */
		if (POSTFILE_BASENAME(fileinfo->filename)[0]=='Z')
			ent = get_num_records(direct,sizeof(struct fileheader));
		/* add end */
        if ((fd = open(direct, O_RDONLY, 0)) != -1) {
            for (i = ent; i > 0; i--) {
                if (0 == get_record_handle(fd, &xfh, sizeof(xfh), i)) {
                    if (0 == strcmp(xfh.filename, fileinfo->filename)) {
                        ent = i;
                        break;
                    }
                }
            }
            close(fd);
        }
        if (0 == i)
            return PARTUPDATE;
        /*
         * Leeward 99.07.12 added above to fix a big bug
         */

        substitute_record(direct, fileinfo, sizeof(*fileinfo), ent);

        setboardorigin(currboard, 1);
        setboardtitle(currboard, 1);
    }
    return PARTUPDATE;
}

/* Mark POST */
int mark_post(int ent, struct fileheader *fileinfo, char *direct)
{
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_MARK_FLAG, 0);
}

/* stiger, 置顶 */
int zhiding_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if(POSTFILE_BASENAME(fileinfo->filename)[0]=='Z')
		return del_ding(ent,fileinfo,direct);
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_DING_FLAG, 0);
}

int noreply_post(int ent, struct fileheader *fileinfo, char *direct)
{
	/* add by stiger ,20030414, 置顶选择*/
	char ans[4];

	if(!chk_currBM(currBM, currentuser)) return DONOTHING;

    move(t_lines - 1, 0);
    clrtoeol();
#ifdef SMTH
    if (HAS_PERM(currentuser,PERM_SYSOP))
#else
    if (1)
#endif
    getdata(t_lines - 1, 0, "切换: 0)取消 1)不可re标记 2)置顶标记 [1]: ", ans, 3, DOECHO, NULL, true);
    else
	    ans[0]='1';
    if (ans[0]=='0') return FULLUPDATE;
    if (ans[0] == ' ') {
        ans[0] = ans[1];
        ans[1] = 0;
    }
	if(ans[0]=='2') return zhiding_post(ent,fileinfo,direct);
	else change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_NOREPLY_FLAG, 1);

	return FULLUPDATE;
}

int noreply_post_noprompt(int ent, struct fileheader *fileinfo, char *direct)
{
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_NOREPLY_FLAG, 0);
}

int sign_post(int ent, struct fileheader *fileinfo, char *direct)
{
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_SIGN_FLAG, 1);
}

#ifdef FILTER
int censor_post(int ent, struct fileheader *fileinfo, char *direct)
{
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_CENSOR_FLAG, 1);
}
#endif

int set_be_title(int ent, struct fileheader *fileinfo, char *direct);

int del_range(int ent, struct fileheader *fileinfo, char *direct, int mailmode)
  /*
   * 区域删除 
   */
{
    char del_mode[11], num1[11], num2[11];
    char fullpath[STRLEN];
    int inum1, inum2;
    int result;                 /* Leeward: 97.12.15 */
    int idel_mode;              /*haohmaru.99.4.20 */

    if (!strcmp(currboard, "syssecurity")
        || !strcmp(currboard, "junk")
        || !strcmp(currboard, "deleted")
        || strstr(direct, ".THREAD") /*Haohmaru.98.10.16 */ )   /* Leeward : 98.01.22 */
        return DONOTHING;

    if (uinfo.mode == READING && !HAS_PERM(currentuser, PERM_SYSOP))
        if (!chk_currBM(currBM, currentuser)) {
            return DONOTHING;
        }

    if (digestmode == 2)
        return DONOTHING;
    if (digestmode == 4 || digestmode == 5) {
        return DONOTHING;
    }
    if (digestmode >= 2)
        return DONOTHING;       /* disabled by bad 2002.8.16*/
    clear();
    prints("区域删除\n");
    /*
     * Haohmaru.99.4.20.增加可以强制删除被mark文章的功能 
     */
    getdata(1, 0, "删除模式 [0]标记删除 [1]普通删除 [2]强制删除(被mark的文章一起删) (0): ", del_mode, 10, DOECHO, NULL, true);
    idel_mode = atoi(del_mode);
    /*
     * if (idel_mode!=0 || idel_mode!=1)
     * {
     * return FULLUPDATE ;
     * } 
     */
    getdata(2, 0, "首篇文章编号(输入0则仅清除标记为删除的文章): ", num1, 10, DOECHO, NULL, true);
    inum1 = atoi(num1);
    if (inum1 == 0) {
        inum2 = -1;
        goto THERE;
    }
    if (inum1 <= 0) {
        prints("错误编号\n");
        pressreturn();
        return FULLUPDATE;
    }
    getdata(3, 0, "末篇文章编号: ", num2, 10, DOECHO, NULL, true);
    inum2 = atoi(num2);
    if (inum2 <= inum1) {
        prints("错误编号\n");
        pressreturn();
        return FULLUPDATE;
    }
  THERE:
    getdata(4, 0, "确定删除 (Y/N)? [N]: ", num1, 10, DOECHO, NULL, true);
    if (*num1 == 'Y' || *num1 == 'y') {
        bmlog(currentuser->userid, currboard, 5, 1);
        result = delete_range(direct, inum1, inum2, idel_mode);
        if (inum1 != 0)
            fixkeep(direct, inum1, inum2);
        else
            fixkeep(direct, 1, 1);
        if (uinfo.mode != RMAIL) {
            updatelastpost(currboard);
            bmlog(currentuser->userid, currboard, 8, inum2-inum1);
            newbbslog(BBSLOG_USER, "del %d-%d on %s", inum1, inum2, currboard);
        }
        prints("删除%s\n", result ? "失败！" : "完成"); /* Leeward: 97.12.15 */
        pressreturn();
        return DIRCHANGED;
    }
    prints("Delete Aborted\n");
    pressreturn();
    return FULLUPDATE;
}

/* add by stiger,delete 置顶文章 */
int del_ding(int ent, struct fileheader *fileinfo, char *direct)
{

	int failed;
	int tmpname[100];

	if ( digestmode != 0 ) return DONOTHING;

    if (!HAS_PERM(currentuser, PERM_SYSOP) && !chk_currBM(currBM, currentuser))
            return DONOTHING;

        clear();
        prints("删除文章 '%s'.", fileinfo->title);
        getdata(1, 0, "(Y/N) [N]: ", genbuf, 3, DOECHO, NULL, true);
        if (genbuf[0] != 'Y' && genbuf[0] != 'y') {     /* if not yes quit */
            move(2, 0);
            prints("取消\n");
            pressreturn();
            clear();
            return FULLUPDATE;
        }

	failed=delete_record(direct, sizeof(struct fileheader), ent, (RECORD_FUNC_ARG) cmpname, fileinfo->filename);

	if(failed){
        move(2, 0);
        prints("删除失败\n");
        pressreturn();
        clear();
		return FULLUPDATE;
	}else{
		sprintf(tmpname,"boards/%s/%s",currboard,fileinfo->filename);
		unlink(tmpname);
	}

	return DIRCHANGED;
}
/* add end */

int del_post(int ent, struct fileheader *fileinfo, char *direct)
{
    char usrid[STRLEN];
    int owned, keep, olddigestmode = 0;
    struct fileheader mkpost;
    extern int SR_BMDELFLAG;

	/* add by stiger */
	if(POSTFILE_BASENAME(fileinfo->filename)[0]=='Z')
		return del_ding(ent,fileinfo,direct);

    if (!strcmp(currboard, "syssecurity")
        || !strcmp(currboard, "junk")
        || !strcmp(currboard, "deleted"))       /* Leeward : 98.01.22 */
        return DONOTHING;

    if (digestmode == 4 || digestmode == 5)
        return DONOTHING;
    keep = sysconf_eval("KEEP_DELETED_HEADER", 0);      /*是否保持被删除的POST的 title */
    if (fileinfo->owner[0] == '-' && keep > 0 && !SR_BMDELFLAG) {
        clear();
        prints("本文章已删除.\n");
        pressreturn();
        clear();
        return FULLUPDATE;
    }
    owned = isowner(currentuser, fileinfo);
    /*
     * change by KCN  ! strcmp( fileinfo->owner, currentuser->userid ); 
     */
    strcpy(usrid, fileinfo->owner);
    if (!(owned) && !HAS_PERM(currentuser, PERM_SYSOP))
        if (!chk_currBM(currBM, currentuser)) {
            return DONOTHING;
        }
    if (!SR_BMDELFLAG) {
        clear();
        prints("删除文章 '%s'.", fileinfo->title);
        getdata(1, 0, "(Y/N) [N]: ", genbuf, 3, DOECHO, NULL, true);
        if (genbuf[0] != 'Y' && genbuf[0] != 'y') {     /* if not yes quit */
            move(2, 0);
            prints("取消\n");
            pressreturn();
            clear();
            return FULLUPDATE;
        }
    }

    if (digestmode != 0 && digestmode != 1) {
        olddigestmode = digestmode;
        digestmode = 0;
        setbdir(digestmode, direct, currboard);
        ent = search_record(direct, &mkpost, sizeof(struct fileheader), (RECORD_FUNC_ARG) cmpfileinfoname, fileinfo->filename);
    }

    if (do_del_post(currentuser, ent, fileinfo, direct, currboard, digestmode, !B_to_b) != 0) {
        move(2, 0);
        prints("删除失败\n");
        pressreturn();
        clear();
        if (olddigestmode) {
            digestmode = olddigestmode;
            setbdir(digestmode, direct, currboard);
        }
        return FULLUPDATE;
    }
    if (olddigestmode) {
        switch (olddigestmode) {
        case DIR_MODE_THREAD:
            title_mode();
            break;
        case DIR_MODE_MARK:
            marked_mode();
            break;
        case DIR_MODE_ORIGIN:
        case DIR_MODE_AUTHOR:
        case DIR_MODE_TITLE:
            search_mode(olddigestmode, search_data);
            break;
        }
    }
    return DIRCHANGED;
}

/* Added by netty to handle post saving into (0)Announce */
int Save_post(int ent, struct fileheader *fileinfo, char *direct)
{
    if (!HAS_PERM(currentuser, PERM_SYSOP))
        if (!chk_currBM(currBM, currentuser))
            return DONOTHING;
    return (a_Save(NULL, currboard, fileinfo, false, direct, ent));
}

/* Semi_save 用来把文章存到暂存档，同时删除文章的头尾 Life 1997.4.6 */
int Semi_save(int ent, struct fileheader *fileinfo, char *direct)
{
    if (!HAS_PERM(currentuser, PERM_SYSOP))
        if (!chk_currBM(currBM, currentuser))
            return DONOTHING;
    return (a_SeSave("0Announce", currboard, fileinfo, false,direct,ent,1));
}

/* Added by netty to handle post saving into (0)Announce */
int Import_post(int ent, struct fileheader *fileinfo, char *direct)
{
    char szBuf[STRLEN];

    if (!HAS_PERM(currentuser, PERM_SYSOP))
        if (!chk_currBM(currBM, currentuser))
            return DONOTHING;

    if (fileinfo->accessed[0] & FILE_IMPORTED) {        /* Leeward 98.04.15 */
        a_prompt(-1, "本文曾经被收录进精华区过. 现在再次收录吗? (Y/N) [N]: ", szBuf);
        if (szBuf[0] != 'y' && szBuf[0] != 'Y')
            return FULLUPDATE;
    }
    /*
     * Leeward 98.04.15 
     */
    a_Import(NULL, currboard, fileinfo, false, direct, ent);
    return FULLUPDATE;
}

int show_b_note()
{
    clear();
    if (show_board_notes(currboard) == -1) {
        move(3, 30);
        prints("此讨论区尚无「备忘录」。");
    }
    pressanykey();
    return FULLUPDATE;
}

#ifdef NINE_BUILD
int show_sec_board_notes(char bname[30])
{                               /* 显示版主的话 */
    char buf[256];

    sprintf(buf, "vote/%s/secnotes", bname);    /* 显示本版的版主的话 vote/版名/notes */
    if (dashf(buf)) {
        ansimore2(buf, false, 0, 23 /*19 */ );
        return 1;
    } else if (dashf("vote/secnotes")) {        /* 显示系统的话 vote/notes */
        ansimore2("vote/secnotes", false, 0, 23 /*19 */ );
        return 1;
    }
    return -1;
}

int show_sec_b_note()
{
    clear();
    if (show_sec_board_notes(currboard) == -1) {
        move(3, 30);
        prints("此讨论区尚无「秘密备忘录」。");
    }
    pressanykey();
    return FULLUPDATE;
}
#endif

int into_announce()
{
    if (a_menusearch("0Announce", currboard, (HAS_PERM(currentuser, PERM_ANNOUNCE) || HAS_PERM(currentuser, PERM_SYSOP) || HAS_PERM(currentuser, PERM_OBOARDS)) ? PERM_BOARDS : 0))
        return FULLUPDATE;
    return DONOTHING;
}

extern int mainreadhelp();
extern int b_results();
extern int b_vote();
extern int b_vote_maintain();
extern int b_notes_edit();
extern int b_sec_notes_edit();
extern int b_jury_edit();       /*stephen 2001.11.1 */

static int sequent_ent;

int sequent_messages(struct fileheader *fptr, int idc, int *continue_flag)
{
    if (readpost) {
        if (idc < sequent_ent)
            return 0;
        if (!brc_unread(fptr->id))
            return 0;           /*已读 则 返回 */
        if (*continue_flag != 0) {
            genbuf[0] = 'y';
        } else {
            prints("讨论区: '%s' 标题:\n\"%s\" posted by %s.\n", currboard, fptr->title, fptr->owner);
            getdata(3, 0, "读取 (Y/N/Quit) [Y]: ", genbuf, 5, DOECHO, NULL, true);
        }
        if (genbuf[0] != 'y' && genbuf[0] != 'Y' && genbuf[0] != '\0') {
            if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
                clear();
                return QUIT;
            }
            clear();
            return 0;
        }
        strncpy(quote_user, fptr->owner, OWNER_LEN);
        quote_user[OWNER_LEN - 1] = 0;
        setbfile(genbuf, currboard, fptr->filename);
        register_attach_link(board_attach_link, fptr);
        ansimore_withzmodem(genbuf, false, fptr->title);
        register_attach_link(NULL,NULL);
      redo:
        move(t_lines - 1, 0);
        clrtoeol();
        prints("\033[1;44;31m[连续读信]  \033[33m回信 R │ 结束 Q,← │下一封 ' ',↓ │^R 回信给作者                \033[m");
        *continue_flag = 0;
        switch (igetkey()) {
        case Ctrl('Y'):
            zsend_post(0, fptr, currdirect);
            clear();
            goto redo;
        case Ctrl('Z'):
            r_lastmsg();        /* Leeward 98.07.30 support msgX */
            break;
        case 'N':
        case 'Q':
        case 'n':
        case 'q':
        case KEY_LEFT:
            break;
        case KEY_REFRESH:
            break;
        case 'Y':
        case 'R':
        case 'y':
        case 'r':
            do_reply(fptr);     /*回信 */
        case ' ':
        case '\n':
        case KEY_DOWN:
            *continue_flag = 1;
            break;
        case Ctrl('R'):
            post_reply(0, fptr, (char *) NULL);
            break;
        default:
            break;
        }
        clear();
    }
    setbdir(digestmode, genbuf, currboard);
    brc_add_read(fptr->id);
    /*
     * return 0;  modified by dong , for clear_new_flag(), 1999.1.20
     * if (strcmp(CurArticleFileName, fptr->filename) == 0)
     * return QUIT;
     * else 
     */
    return 0;

}

int sequential_read(int ent, struct fileheader *fileinfo, char *direct)
{
    readpost = 1;
    clear();
    return sequential_read2(ent);
}
 /*ARGSUSED*/ int sequential_read2(int ent)
{
    char buf[STRLEN];
    int continue_flag;

    sequent_ent = ent;
    continue_flag = 0;
    setbdir(digestmode, buf, currboard);
    apply_record(buf, (APPLY_FUNC_ARG) sequent_messages, sizeof(struct fileheader), &continue_flag, 1, false);
    return FULLUPDATE;
}

int clear_new_flag(int ent, struct fileheader *fileinfo, char *direct)
{
	/* add by stiger */
	if(POSTFILE_BASENAME(fileinfo->filename)[0]=='Z') brc_clear();
	else brc_clear_new_flag(fileinfo->id);
    return PARTUPDATE;
}

int clear_all_new_flag(int ent, struct fileheader *fileinfo, char *direct)
{
    brc_clear();
    return PARTUPDATE;
}

int show_t_friends()
{
    if (!HAS_PERM(currentuser, PERM_BASIC))
        return PARTUPDATE;
    t_friends();
    return FULLUPDATE;
}

struct one_key read_comms[] = { /*阅读状态，键定义 */
    {'r', read_post},
    {'K', skip_post},
    /*
     * {'u',        skip_post},    rem by Haohmaru.99.11.29 
     */
    {'d', del_post},
    {'D', del_range},
    {'m', mark_post},
    {';', noreply_post},        /*Haohmaru.99.01.01,设定不可re模式 */
    {'#', sign_post},           /* Bigman: 2000.8.12  设定文章标记模式 */
#ifdef FILTER
    {'@', censor_post},         /* czz: 2002.9.29 审核被过滤文章 */
#endif
    {'E', edit_post},
    {Ctrl('G'), change_mode},   /* bad : 2002.8.8 add marked mode */
    {'H', read_hot_info},   /* flyriver: 2002.12.21 增加热门信息显示 */
    {'`', digest_mode},
    {'.', deleted_mode},
    {'>', junk_mode},
    {'g', digest_post},
    {'T', edit_title},
    {'s', do_select},
    {Ctrl('C'), do_cross},
    {'Y', UndeleteArticle},     /* Leeward 98.05.18 */
    {Ctrl('P'), do_post},
#ifdef NINE_BUILD
    {'c', show_t_friends},
    {'C', clear_new_flag},
#else
    {'c', clear_new_flag},
#endif
    {'f', clear_all_new_flag},  /* added by dong, 1999.1.25 */
    {'S', sequential_read},
#ifdef INTERNET_EMAIL
    {'F', mail_forward},
    {'U', mail_uforward},
    {Ctrl('R'), post_reply},
#endif
    {'J', Semi_save},
    {'i', Save_post},
    {'I', Import_post},
    {'R', b_results},
    {'V', b_vote},
    {'M', b_vote_maintain},
    {'W', b_notes_edit},
    {'h', mainreadhelp},
    {'X', b_jury_edit},
/*编辑版面的仲裁委员名单,stephen on 2001.11.1 */
    {KEY_TAB, show_b_note},
    {'x', into_announce},
    {'a', auth_search_down},
    {'A', auth_search_up},
    {'/', t_search_down},
    {'?', t_search_up},
    {'\'', post_search_down},
    {'\"', post_search_up},
    {']', thread_down},
    {'[', thread_up},
    {Ctrl('D'), deny_user},
    {Ctrl('E'), clubmember},
    {Ctrl('A'), show_author},
    {Ctrl('O'), add_author_friend},
    {Ctrl('Q'), show_authorinfo},       /*Haohmaru.98.12.05 */
    {Ctrl('W'), show_authorBM}, /*cityhunter 00.10.18 */
#ifdef NINE_BUILD
    {'z', show_sec_b_note},     /*Haohmaru.2000.5.19 */
    {'Z', b_sec_notes_edit},
#else
    {'z', sendmsgtoauthor},     /*Haohmaru.2000.5.19 */
    {'Z', sendmsgtoauthor},     /*Haohmaru.2000.5.19 */
#endif
    {Ctrl('N'), SR_first_new},
    {'n', SR_first_new},
    {'\\', SR_last},
    {'|', set_be_title},
    {'=', SR_first},
    {Ctrl('S'), SR_read},
    {'p', SR_read},
    {Ctrl('X'), SR_readX},      /* Leeward 98.10.03 */
    {Ctrl('U'), SR_author},
    {Ctrl('H'), SR_authorX},    /* Leeward 98.10.03 */
    {'b', SR_BMfunc},
    {'B', SR_BMfuncX},          /* Leeward 98.04.16 */
    {Ctrl('T'), title_mode},
    {'t', set_delete_mark},     /*KCN 2001 */
    {'v', i_read_mail},         /* period 2000-11-12 read mail in article list */
    /*
     * {'!',      Goodbye},Haohmaru 98.09.21 
     */
    {Ctrl('Y'), zsend_post},    /* COMMAN 2002 */
    {'\0', NULL},
};

int Read()
{
    char buf[2 * STRLEN];
    char notename[STRLEN];
    time_t usetime;
    struct stat st;

    if (!selboard) {
        move(2, 0);
        prints("请先选择讨论区\n");
        pressreturn();
        move(2, 0);
        clrtoeol();
        return -1;
    }
    in_mail = false;
    brc_initial(currentuser->userid, currboard);
    setbdir(digestmode, buf, currboard);

    setvfile(notename, currboard, "notes");
    if (stat(notename, &st) != -1) {
        if (st.st_mtime < (time(NULL) - 7 * 86400)) {
/*            sprintf(genbuf,"touch %s",notename);
	    */
            f_touch(notename);
            setvfile(genbuf, currboard, "noterec");
            unlink(genbuf);
        }
    }
    if (vote_flag(currboard, '\0', 1 /*检查读过新的备忘录没 */ ) == 0) {
        if (dashf(notename)) {
            /*
             * period  2000-09-15  disable ActiveBoard while reading notes 
             */
            modify_user_mode(READING);
            /*-	-*/
            ansimore(notename, true);
            vote_flag(currboard, 'R', 1 /*写入读过新的备忘录 */ );
        }
    }
    usetime = time(0);
    i_read(READING, buf, readtitle, (READ_FUNC) readdoent, &read_comms[0], sizeof(struct fileheader));  /*进入本版 */
    newbbslog(BBSLOG_BOARDUSAGE, "%-20s Stay: %5ld", currboard, time(0) - usetime);
    bmlog(currentuser->userid, currboard, 0, time(0) - usetime);
    bmlog(currentuser->userid, currboard, 1, 1);

    return 0;
}

/*Add by SmallPig*/
static int catnotepad(FILE * fp, char *fname)
{
    char inbuf[256];
    FILE *sfp;
    int count;

    count = 0;
    if ((sfp = fopen(fname, "r")) == NULL) {
        fprintf(fp, "[31m[41m⊙┴———————————————————————————————————┴⊙[m\n\n");
        return -1;
    }
    while (fgets(inbuf, sizeof(inbuf), sfp) != NULL) {
        if (count != 0)
            fputs(inbuf, fp);
        else
            count++;
    }
    fclose(sfp);
    return 0;
}

void notepad()
{
    char tmpname[STRLEN], note1[4];
    char note[3][STRLEN - 4];
    char tmp[STRLEN];
    FILE *in;
    int i, n;
    time_t thetime = time(0);

    clear();
    move(0, 0);
    prints("开始你的留言吧！大家正拭目以待....\n");
    sprintf(tmpname, "etc/notepad_tmp/%s.notepad", currentuser->userid);
    if ((in = fopen(tmpname, "w")) != NULL) {
        for (i = 0; i < 3; i++)
            memset(note[i], 0, STRLEN - 4);
        while (1) {
            for (i = 0; i < 3; i++) {
                getdata(1 + i, 0, ": ", note[i], STRLEN - 5, DOECHO, NULL, false);
                if (note[i][0] == '\0')
                    break;
            }
            if (i == 0) {
                fclose(in);
                unlink(tmpname);
                return;
            }
            getdata(5, 0, "是否把你的大作放入留言板 (Y)是的 (N)不要 (E)再编辑 [Y]: ", note1, 3, DOECHO, NULL, true);
            if (note1[0] == 'e' || note1[0] == 'E')
                continue;
            else
                break;
        }
        if (note1[0] != 'N' && note1[0] != 'n') {
            sprintf(tmp, "[32m%s[37m（%.24s）", currentuser->userid, currentuser->username);
            fprintf(in, "[31m[40m⊙┬——————————————┤[37m酸甜苦辣板[31m├——————————————┬⊙[m\n");
            fprintf(in, "[31m□┤%-43s[33m在 [36m%.19s[33m 离开时留下的话[31m├□\n", tmp, Ctime(thetime));
            if (i > 2)
                i = 2;
            for (n = 0; n <= i; n++) {
#ifdef FILTER
	        if (check_badword_str(note[n],strlen(note[n]))) {
			int t;
                        for (t = n; t <= i; t++) 
                            fprintf(in, "[31m│[m%-74.74s[31m│[m\n", note[t]);
			fclose(in);

                        post_file(currentuser, "", tmpname, FILTER_BOARD, "---留言版过滤器---", 0, 2);

			unlink(tmpname);
			return;
		}
#endif
                if (note[n][0] == '\0')
                    break;
                fprintf(in, "[31m│[m%-74.74s[31m│[m\n", note[n]);
            }
            fprintf(in, "[31m□┬———————————————————————————————————┬□[m\n");
            catnotepad(in, "etc/notepad");
            fclose(in);
            f_mv(tmpname, "etc/notepad");
        } else {
            fclose(in);
            unlink(tmpname);
        }
    }
    if (talkrequest) {
        talkreply();
    }
    clear();
    return;
}

void record_exit_time()
{                               /* 记录离线时间  Luzi 1998/10/23 */
    currentuser->exittime = time(NULL);
    /*
     * char path[80];
     * FILE *fp;
     * time_t now;
     * sethomefile( path, currentuser->userid , "exit");
     * fp=fopen(path, "wb");
     * if (fp!=NULL)
     * {
     * now=time(NULL);
     * fwrite(&now,sizeof(time_t),1,fp);
     * fclose(fp);
     * }
     */
}

extern int icurrchar, ibufsize;

int Goodbye()
{                               /*离站 选单 */
    extern int started;
    time_t stay;
    char fname[STRLEN], notename[STRLEN];
    char sysoplist[20][STRLEN], syswork[20][STRLEN], spbuf[STRLEN], buf[STRLEN];
    int i, num_sysop, choose, logouts, mylogout = false;
    FILE *sysops;
    long Time = 10;             /*Haohmaru */
    int left = (80 - 36) / 2;
    int top = (scr_lns - 11) / 2;
    struct _select_item level_conf[] = {
        {left + 7, top + 2, -1, SIT_SELECT, (void *) ""},
        {left + 7, top + 3, -1, SIT_SELECT, (void *) ""},
        {left + 7, top + 4, -1, SIT_SELECT, (void *) ""},
        {left + 7, top + 5, -1, SIT_SELECT, (void *) ""},
        {-1, -1, -1, 0, NULL}
    };

/*---	显示备忘录的关掉该死的活动看板	2001-07-01	---*/
    modify_user_mode(READING);

    i = 0;
    if ((sysops = fopen("etc/sysops", "r")) != NULL) {
        while (fgets(buf, STRLEN, sysops) != NULL && i < 20) {
            strcpy(sysoplist[i], (char *) strtok(buf, " \n\r\t"));
            strcpy(syswork[i], (char *) strtok(NULL, " \n\r\t"));
            i++;
        }
        fclose(sysops);
    }
    num_sysop = i;
    move(1, 0);
    clear();
    move(top, left);
    outs("\x1b[1;47;37m╔═[*]═══ Message ══════╗\x1b[m");
    move(top + 1, left);
    outs("\x1b[1;47;37m║\x1b[44;37m                                \x1b[47;37m║\x1b[m");
    move(top + 2, left);
    prints("\x1b[1;47;37m║\x1b[44;37m     [\x1b[33m1\x1b[37m] 寄信给%-10s       \x1b[47;37m║\x1b[m", NAME_BBS_CHINESE);
    move(top + 3, left);
    prints("\x1b[1;47;37m║\x1b[44;37m     [\x1b[33m2\x1b[37m] \x1b[32m返回%-15s\x1b[37m    \x1b[47;37m║\x1b[m", NAME_BBS_CHINESE "BBS站");
    move(top + 4, left);
    outs("\x1b[1;47;37m║\x1b[44;37m     [\x1b[33m3\x1b[37m] 写写*留言板*           \x1b[47;37m║\x1b[m");
    move(top + 5, left);
    outs("\x1b[1;47;37m║\x1b[44;37m     [\x1b[33m4\x1b[37m] 离开本BBS站            \x1b[47;37m║\x1b[m");
    move(top + 6, left);
    outs("\x1b[1;47;37m║\x1b[0;44;34m________________________________\x1b[1;47;37m║\x1b[m");
    move(top + 7, left);
    outs("\x1b[1;47;37m║                                ║\x1b[m");
    move(top + 8, left);
    outs("\x1b[1;47;37m║        \x1b[42;33m  取消(两下ESC) \x1b[0;47;30m▃\x1b[1;37m      ║\x1b[m");
    move(top + 9, left);
    outs("\x1b[1;47;37m║          \x1b[0;40;37m▃▃▃▃▃▃▃▃\x1b[1;47;37m      ║\x1b[m");
    move(top + 10, left);
    outs("\x1b[1;47;37m╚════════════════╝\x1b[m");
    outs("\x1b[1;44;37m");

    choose = simple_select_loop(level_conf, SIF_SINGLE | SIF_ESCQUIT | SIF_NUMBERKEY, t_columns, t_lines, NULL);
    if (choose == 0)
        choose = 2;
    clear();
    if (strcmp(currentuser->userid, "guest") && choose == 1) {  /* 写信给站长 */
        if (PERM_LOGINOK & currentuser->userlevel) {    /*Haohmaru.98.10.05.没通过注册的只能给注册站长发信 */
            prints("        ID        负责的职务\n");
            prints("   ============ =============\n");
            for (i = 1; i <= num_sysop; i++) {
                prints("[[33m%1d[m] [1m%-12s %s[m\n", i, sysoplist[i - 1], syswork[i - 1]);
            }

            prints("[[33m%1d[m] 还是走了罗！\n", num_sysop + 1);      /*最后一个选项 */

            sprintf(spbuf, "你的选择是 [[32m%1d[m]：", num_sysop + 1);
            getdata(num_sysop + 5, 0, spbuf, genbuf, 4, DOECHO, NULL, true);
            choose = genbuf[0] - '0';
            if (0 != genbuf[1])
                choose = genbuf[1] - '0' + 10;
            if (choose >= 1 && choose <= num_sysop) {
                /*
                 * do_send(sysoplist[choose-1], "使用者寄来的的建议信"); 
                 */
                if (choose == 1)        /*modified by Bigman : 2000.8.8 */
                    do_send(sysoplist[0], "【站务总管】使用者寄来的建议信", "");
                else if (choose == 2)
                    do_send(sysoplist[1], "【系统维护】使用者寄来的建议信", "");
                else if (choose == 3)
                    do_send(sysoplist[2], "【版面管理】使用者寄来的建议信", "");
                else if (choose == 4)
                    do_send(sysoplist[3], "【身份确认】使用者寄来的建议信", "");
                else if (choose == 5)
                    do_send(sysoplist[4], "【仲裁事宜】使用者寄来的建议信", "");
            }
/* added by stephen 11/13/01 */
            choose = -1;
        } else {
            /*
             * 增加注册的提示信息 Bigman:2000.10.31 
             */
            prints("\n    如果您一直未得到身份认证,请确认您是否到个人工具箱填写了注册单,\n");
            prints("    如果您收到身份确认信,还没有发文聊天等权限,请试着再填写一遍注册单\n\n");
            prints("     站长的 ID   负责的职务\n");
            prints("   ============ =============\n");

            /*
             * added by Bigman: 2000.8.8  修改离站 
             */
            prints("[[33m%1d[m] [1m%-12s %s[m\n", 1, sysoplist[3], syswork[3]);
            prints("[[33m%1d[m] 还是走了罗！\n", 2);  /*最后一个选项 */

            sprintf(spbuf, "你的选择是 %1d：", 2);
            getdata(num_sysop + 6, 0, spbuf, genbuf, 4, DOECHO, NULL, true);
            choose = genbuf[0] - '0';
            if (choose == 1)    /*modified by Bigman : 2000.8.8 */
                do_send(sysoplist[3], "【身份确认】使用者寄来的建议信", "");
            choose = -1;

            /*
             * for(i=0;i<=3;i++)
             * prints("[[33m%1d[m] [1m%-12s %s[m\n",
             * i,sysoplist[i+4],syswork[i+4]);
             * prints("[[33m%1d[m] 还是走了罗！\n",4); 
 *//*
 * * * * * * * * * * * 最后一个选项 
 */
            /*
             * sprintf(spbuf,"你的选择是 [[32m%1d[m]：",4);
             * getdata(num_sysop+6,0, spbuf,genbuf, 4, DOECHO, NULL ,true);
             * choose=genbuf[0]-'0';
             * if(choose==1)
             * do_send(sysoplist[5], "使用者寄来的的建议信");
             * else if(choose==2)
             * do_send(sysoplist[6], "使用者寄来的的建议信");
             * else if(choose==3)
             * do_send(sysoplist[7], "使用者寄来的的建议信");
             * else if(choose==0)
             * do_send(sysoplist[4], "使用者寄来的的建议信");
             * choose=-1; 
             */
        }
    }
    if (choose == 2)            /*返回BBS */
        return 0;
    if (strcmp(currentuser->userid, "guest") != 0) {
        if (choose == 3)        /*留言簿 */
            if (USE_NOTEPAD == 1 && HAS_PERM(currentuser, PERM_POST))
                notepad();
    }

    clear();
    prints("\n\n\n\n");
    stay = time(NULL) - login_start_time;       /*本次线上时间 */

    currentuser->stay += stay;

    if (DEFINE(currentuser, DEF_OUTNOTE /*退出时显示用户备忘录 */ )) {
        sethomefile(notename, currentuser->userid, "notes");
        if (dashf(notename))
            ansimore(notename, true);
    }

    /*
     * Leeward 98.09.24 Use SHARE MEM and disable the old code 
     */
    if (DEFINE(currentuser, DEF_LOGOUT)) {      /* 使用自己的离站画面 */
        sethomefile(fname, currentuser->userid, "logout");
        if (dashf(fname))
            mylogout = true;
    }
    if (mylogout) {
        logouts = countlogouts(fname);  /* logouts 为 离站画面 总数 */
        if (logouts >= 1) {
            user_display(fname, (logouts == 1) ? 1 : (currentuser->numlogins % (logouts)) + 1, true);
        }
    } else {
        logouts = countlogouts("etc/logout");   /* logouts 为 离站画面 总数 */
        user_display("etc/logout", rand() % logouts + 1, true);
    }

    /*
     * if(DEFINE(currentuser,DEF_LOGOUT\*使用自己的离站画面*\)) Leeward: disable the old code
     * {
     * sethomefile( fname,currentuser->userid, "logout" );
     * if(!dashf(fname))
     * strcpy(fname,"etc/logout");
     * }else
     * strcpy(fname,"etc/logout");
     * if(dashf(fname))
     * {
     * logouts=countlogouts(fname);      \* logouts 为 离站画面 总数 *\
     * if(logouts>=1)
     * {
     * user_display(fname,(logouts==1)?1:
     * (currentuser->numlogins%(logouts))+1,true);
     * }
     * } 
     */
    bbslog("user", "%s", "exit");

    /*
     * stay = time(NULL) - login_start_time;    本次线上时间 
     */
    /*
     * Haohmaru.98.11.10.简单判断是否用上站机 
     */
    if ( /*strcmp(currentuser->username,"guest")&& */ stay <= Time) {
        char lbuf[256];
        char tmpfile[256];
        FILE *fp;

        strcpy(lbuf, "自首-");
        strftime(lbuf + 5, 30, "%Y-%m-%d%Y:%H:%M", localtime(&login_start_time));
        sprintf(tmpfile, "tmp/.tmp%d", getpid());
        fp = fopen(tmpfile, "w");
        if (fp) {
            fputs(lbuf, fp);
            fclose(fp);
            mail_file(currentuser->userid, tmpfile, "surr", "自首", BBSPOST_MOVE, NULL);
        }
    }
    /*
     * stephen on 2001.11.1: 上站不足5分钟不计算上站次数 
     */
    if (stay <= 300 && currentuser->numlogins > 5) {
        currentuser->numlogins--;
        if (currentuser->stay > stay)
            currentuser->stay -= stay;
    }
    if (started) {
        record_exit_time();     /* 记录用户的退出时间 Luzi 1998.10.23 */
        /*---	period	2000-10-19	4 debug	---*/
        /*
         * sprintf( genbuf, "Stay:%3ld (%s)", stay / 60, currentuser->username ); 
         */
        newbbslog(BBSLOG_USIES, "EXIT: Stay:%3ld (%s)[%d %d]", stay / 60, currentuser->username, utmpent, usernum);
        u_exit();
        started = 0;
    }

    if (num_user_logins(currentuser->userid) == 0 || !strcmp(currentuser->userid, "guest")) {   /*检查还有没有人在线上 */
        FILE *fp;
        char buf[STRLEN], *ptr;

//        sethomefile(fname, currentuser->userid, "msgfile");
        if (DEFINE(currentuser, DEF_MAILMSG /*离站时寄回所有信息 */ ) && (get_msgcount(0, currentuser->userid))) {
                mail_msg(currentuser);
/*    #ifdef NINE_BUILD
            time_t now, timeout;
            char ans[3];

            timeout = time(0) + 60;
            do {
                move(t_lines - 1, 0);
                clrtoeol();
                getdata(t_lines - 1, 0, "是否将此次所收到的所有讯息存档 (Y/N)? ", ans, 2, DOECHO, NULL, true);
                if ((toupper(ans[0]) == 'Y') || (toupper(ans[0]) == 'N'))
                    break;
            } while (time(0) < timeout);
            if (toupper(ans[0]) == 'Y') {
#endif
                char title[STRLEN];
                time_t now;

                now = time(0);
                sprintf(title, "[%12.12s] 所有讯息备份", ctime(&now) + 4);
                mail_file(currentuser->userid, fname, currentuser->userid, title, BBSPOST_MOVE);
#ifdef NINE_BUILD
            }
#endif*/
        } else
            clear_msg(currentuser->userid);
        fp = fopen("friendbook", "r");  /*搜索系统 寻人名单 */
        while (fp != NULL && fgets(buf, sizeof(buf), fp) != NULL) {
            char uid[14];

            ptr = strstr(buf, "@");
            if (ptr == NULL) {
                del_from_file("friendbook", buf);
                continue;
            }
            ptr++;
            strcpy(uid, ptr);
            ptr = strstr(uid, "\n");
            *ptr = '\0';
            if (!strcmp(uid, currentuser->userid))      /*删除本用户的 寻人名单 */
                del_from_file("friendbook", buf);       /*寻人名单只在本次上线有效 */
        }
        if (fp)                                                                                        /*---	add by period 2000-11-11 fix null hd bug	---*/
            fclose(fp);
    }
    sleep(1);
    pressreturn();              /*Haohmaru.98.10.18 */
    prints("\x1b[m");
    clear();
    icurrchar = 0; ibufsize = 0;
    refresh();
    shutdown(0, 2);
    close(0);
    exit(0);
    return -1;
}



int Info()
{                               /* 显示版本信息Version.Info */
    modify_user_mode(XMENU);
    ansimore("Version.Info", true);
    clear();
    return 0;
}

int Conditions()
{                               /* 显示版权信息COPYING */
    modify_user_mode(XMENU);
    ansimore("COPYING", true);
    clear();
    return 0;
}

int ShowWeather()
{                               /* 显示版本信息Version.Info */
    modify_user_mode(XMENU);
    ansimore("WEATHER", true);
    clear();
    return 0;
}

int Welcome()
{                               /* 显示欢迎画面 Welcome */
    modify_user_mode(XMENU);
    ansimore("Welcome", true);
    clear();
    return 0;
}

int cmpbnames(char *bname, struct fileheader *brec)
{
    if (!strncasecmp(bname, brec->filename, sizeof(brec->filename)))
        return 1;
    else
        return 0;
}

void RemoveAppendedSpace(char *ptr)
{                               /* Leeward 98.02.13 */
    int Offset;

    /*
     * Below block removing extra appended ' ' in article titles 
     */
    Offset = strlen(ptr);
    for (--Offset; Offset > 0; Offset--) {
        if (' ' != ptr[Offset])
            break;
        else
            ptr[Offset] = 0;
    }
}

int i_read_mail()
{
    char savedir[STRLEN];

    /*
     * should set digestmode to false while read mail. or i_read may cause error 
     */
    int savemode;

    if(!HAS_PERM(currentuser, PERM_BASIC)||!strcmp(currentuser->userid, "guest")) return DONOTHING;
    strcpy(savedir, currdirect);
    savemode = digestmode;
    digestmode = false;
	if (HAS_MAILBOX_PROP(&uinfo, MBP_MAILBOXSHORTCUT))
		MailProc();
	else
    	m_read();
    digestmode = savemode;
    strcpy(currdirect, savedir);
    return FULLUPDATE;
}

int set_delete_mark(int ent, struct fileheader *fileinfo, char *direct)
{
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_DELETE_FLAG, 1);
}

int set_be_title(int ent, struct fileheader *fileinfo, char *direct)
{
    /*
     * 设置该文件为标题文件---- added by bad 2002.8.14
     */
    return change_post_flag(currBM, currentuser, digestmode, currboard, ent, fileinfo, direct, FILE_TITLE_FLAG, 0);
}

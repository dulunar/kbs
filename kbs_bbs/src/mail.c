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

#include "bbs.h"
#define         INTERNET_PRIVATE_EMAIL

/*For read.c*/
int auth_search_down();
int auth_search_up();
int do_cross();
int t_search_down();
int t_search_up();
int thread_up();
int thread_down();
int deny_user();
int show_author();
int show_authorinfo();          /*Haohmaru.98.12.19 */
int show_authorBM();            /*cityhunter 00.10.18 */
int SR_first_new();
int SR_last();
int SR_first();
int SR_read();
int SR_readX();                 /* Leeward 98.10.03 */
int SR_author();
int SR_authorX();               /* Leeward 98.10.03 */
int G_SENDMODE = false;
extern int add_author_friend();
int cmpinames();                /* added by Leeward 98.04.10 */

extern int nf;
extern int numofsig;
extern char quote_user[];
char *sysconf_str();
char currmaildir[STRLEN];
extern char mail_list[MAILBOARDNUM][40];
extern int mail_list_t;

#define maxrecp 300

static int mail_reply(int ent, struct fileheader *fileinfo, char *direct);
static int mail_del(int ent, struct fileheader *fileinfo, char *direct);
static int do_gsend(char *userid[], char *title, int num);

int chkmail()
{
    static time_t lasttime = 0;
    static int ismail = 0;
    struct fileheader fh;
    struct stat st;
    int fd;
    int i, offset;
    long numfiles;
    unsigned char ch;
    extern char currmaildir[STRLEN];
    int sum, sumlimit, numlimit;        /*Haohmaru.99.4.4.对收信也加限制 */

    m_init();
    if (!HAS_PERM(currentuser, PERM_BASIC)) {
        return 0;
    }
    /*
     * ylsdd 2001.4.23: 检测文件状态应该在get_mailnum，get_sum_records之前，否则岂不是
     * 要做大量无用的系统调用. 在这个改动中也把fstat改为stat了，节省一个open&close 
     */
    if (stat(currmaildir, &st) < 0)
        return (ismail = 0);
    if (lasttime >= st.st_mtime)
        return ismail;


    if (chkusermail(currentuser))
        return (ismail = 2);
    offset = (int) ((char *) &(fh.accessed[0]) - (char *) &(fh));
    if ((fd = open(currmaildir, O_RDONLY)) < 0)
        return (ismail = 0);
    lasttime = st.st_mtime;
    numfiles = st.st_size;
    numfiles = numfiles / sizeof(fh);
    if (numfiles <= 0) {
        close(fd);
        return (ismail = 0);
    }
    lseek(fd, (st.st_size - (sizeof(fh) - offset)), SEEK_SET);
    for (i = 0; i < numfiles; i++) {
        read(fd, &ch, 1);
        if (!(ch & FILE_READ)) {
            close(fd);
            return (ismail = 1);
        }
        lseek(fd, -sizeof(fh) - 1, SEEK_CUR);
    }
    close(fd);
    return (ismail = 0);
}

int get_mailnum()
{
    struct fileheader fh;
    struct stat st;
    int fd;
    register int numfiles;
    extern char currmaildir[STRLEN];

    if ((fd = open(currmaildir, O_RDONLY)) < 0)
        return 0;
    fstat(fd, &st);
    numfiles = st.st_size;
    numfiles = numfiles / sizeof(fh);
    close(fd);
    return numfiles;
}


static int mailto(struct userec *uentp, char *arg)
{
    char filename[STRLEN];
    int mailmode = (int) arg;

    sprintf(filename, "etc/%s.mailtoall", currentuser->userid);
    if ((uentp->userlevel == PERM_BASIC && mailmode == 1) ||
        (uentp->userlevel & PERM_POST && mailmode == 2) || (uentp->userlevel & PERM_BOARDS && mailmode == 3) || (uentp->userlevel & PERM_CHATCLOAK && mailmode == 4)) {
        mail_file(currentuser->userid, filename, uentp->userid, save_title, 0);
    }
    return 1;
}

static int mailtoall(int mode)
{

    return apply_users(mailto, (char *) mode);
}

int mailall()
{
    char ans[4], ans4[4], ans2[4], fname[STRLEN], title[STRLEN];
    char doc[4][STRLEN], buf[STRLEN];
    char buf2[STRLEN], include_mode = 'Y';
    char buf3[STRLEN], buf4[STRLEN];
    int i, replymode = 0;       /* Post New UI */

    strcpy(title, "没主题");
    buf4[0] = '\0';
    modify_user_mode(SMAIL);
    clear();
    move(0, 0);
    sprintf(fname, "etc/%s.mailtoall", currentuser->userid);
    prints("你要寄给所有的：\n");
    prints("(0) 放弃\n");
    strcpy(doc[0], "(1) 未认证身份者");
    strcpy(doc[1], "(2) 已认证身份者");
    strcpy(doc[2], "(3) 有版主权限者");
    strcpy(doc[3], "(4) 智囊团成员");
    for (i = 0; i < 4; i++)
        prints("%s\n", doc[i]);
    while (1) {
        getdata(8, 0, "请输入模式 (0~4)? [0]: ", ans4, 2, DOECHO, NULL, true);

        if (ans4[0] - '0' >= 1 && ans4[0] - '0' <= 4) {
            sprintf(buf, "是否确定寄给%s (Y/N)? [N]: ", doc[ans4[0] - '0' - 1]);
            getdata(9, 0, buf, ans2, 2, DOECHO, NULL, true);
            if (ans2[0] != 'Y' && ans2[0] != 'y') {
                return -1;
            }
            in_mail = true;
            /*
             * Leeward 98.01.17 Prompt whom you are writing to 
             */
            /*
             * strcpy(currentlookupuser->userid, doc[ans4[0]-'0'-1] + 4); 
             */

            if (currentuser->signature > numofsig)
                currentuser->signature = 1;
            while (1) {
                sprintf(buf3, "引言模式 [[1m%c[m]", include_mode);
                move(t_lines - 4, 0);
                clrtoeol();
                prints("收信人: [1m%s[m\n", doc[ans4[0] - '0' - 1]);
                clrtoeol();
                prints("使用标题: [1m%-50s[m\n", (title[0] == '\0') ? "[正在设定标题]" : title);
                clrtoeol();
                if (currentuser->signature < 0)
                    prints("使用随机签名档     %s", (replymode) ? buf3 : "");
                else
                    prints("使用第 [1m%d[m 个签名档     %s", currentuser->signature, (replymode) ? buf3 : "");

                if (buf4[0] == '\0' || buf4[0] == '\n') {
                    move(t_lines - 1, 0);
                    clrtoeol();
                    getdata(t_lines - 1, 0, "标题: ", buf4, 50, DOECHO, NULL, true);
                    if ((buf4[0] == '\0' || buf4[0] == '\n')) {
                        buf4[0] = ' ';
                        continue;
                    }
                    strcpy(title, buf4);
                    continue;
                }
                move(t_lines - 1, 0);
                clrtoeol();
                /*
                 * Leeward 98.09.24 add: viewing signature(s) while setting post head 
                 */
                sprintf(buf2, "按[1;32m0[m~[1;32m%d/V/L[m选/看/随机签名档%s，[1;32mT[m改标题，[1;32mEnter[m接受所有设定: ", numofsig,
                        (replymode) ? "，[1;32mY[m/[1;32mN[m/[1;32mR[m/[1;32mA[m改引言模式" : "");
                getdata(t_lines - 1, 0, buf2, ans, 3, DOECHO, NULL, true);
                ans[0] = toupper(ans[0]);       /* Leeward 98.09.24 add; delete below toupper */
                if ((ans[0] - '0') >= 0 && ans[0] - '0' <= 9) {
                    if (atoi(ans) <= numofsig)
                        currentuser->signature = atoi(ans);
                } else if ((ans[0] == 'Y' || ans[0] == 'N' || ans[0] == 'A' || ans[0] == 'R') && replymode) {
                    include_mode = ans[0];
                } else if (ans[0] == 'T') {
                    buf4[0] = '\0';
                } else if (ans[0] == 'V') {     /* Leeward 98.09.24 add: viewing signature(s) while setting post head */
                    sethomefile(buf2, currentuser->userid, "signatures");
                    move(t_lines - 1, 0);
                    if (askyn("预设显示前三个签名档, 要显示全部吗", false) == true)
                        ansimore(buf2, 0);
                    else {
                        clear();
                        ansimore2(buf2, false, 0, 18);
                    }
                } else if (ans[0] == 'L') {
                    currentuser->signature = -1;
                } else {
                    strncpy(save_title, title, STRLEN);
                    break;
                }
            }
            do_quote(fname, include_mode, "", quote_user);
            if (vedit(fname, true) == -1) {
                in_mail = false;
                unlink(fname);
                clear();
                return -2;
            }
            move(t_lines - 1, 0);
            clrtoeol();
            prints("[32m[44m正在寄信件中，请稍候.....                                                        [m");
            refresh();
            mailtoall(ans4[0] - '0');
            move(t_lines - 1, 0);
            clrtoeol();
            unlink(fname);
            in_mail = false;
            return 0;
        } else
            in_mail = false;
        return 0;
    }
    return -1;
}

void m_internet()
{
    char receiver[STRLEN], title[STRLEN];

    modify_user_mode(SMAIL);
    getdata(1, 0, "收信人: ", receiver, 70, DOECHO, NULL, true);
    getdata(2, 0, "主题  : ", title, 70, DOECHO, NULL, true);
    if (!invalidaddr(receiver) && strchr(receiver, '@') && strlen(title) > 0) {
        clear();                /* Leeward 98.09.24fix a bug */
        switch (do_send(receiver, title, "")) { /* Leeward 98.05.11 adds "switch" */
        case -1:
            prints("收信者不正确\n");
            break;
        case -2:
            prints("取消发信\n");
            break;
        case -3:
            prints("'%s' 无法收信\n", receiver);
            break;
        case -4:
            clear();
            move(1, 0);
            prints("%s 信箱已满,无法收信\n", receiver);
            break;              /*Haohmaru.4.5.收信限制 */
        case -5:
            clear();
            move(1, 0);
            prints("%s 自杀中，不能收信\n", receiver);
            break;              /*Haohmaru.99.10.26.自杀者不能收信 */
        case -552:
            prints("\n[1m[33m信件超长（本站限定信件长度上限为 %d 字节），取消发信操作[m[m\n", MAXMAILSIZE);
            break;
        default:
            prints("信件已寄出\n");
        }
        pressreturn();

    } else {
        move(3, 0);
        prints("收信人或主题不正确, 请重新选取指令\n");
        pressreturn();
    }
    clear();
    refresh();
}

void m_init()
{
    setmailfile(currmaildir, currentuser->userid, DOT_DIR);
}

int do_send(char *userid, char *title, char *q_file)
{
    struct fileheader newmessage;
    struct stat st;
    char filepath[STRLEN], fname[STRLEN], *ip;
    int fp, sum, sumlimit, numlimit;
    char buf2[256], buf3[STRLEN], buf4[STRLEN];
    int replymode = 1;          /* Post New UI */
    char ans[4], include_mode = 'Y';

    int internet_mail = 0;
    char tmp_fname[256];
    int noansi;
    struct userec *user;
    extern char quote_title[120];

    int now;                    /* added by Bigman: for SYSOP mail */
    int ret;

    if (!strchr(userid, '@')) {
        if (getuser(userid, &user) == 0)
            return -4;
        ret = chkreceiver(currentuser, user);
        if (ret == 1)
            return -3;
        /*
         * SYSOP也能给自杀的人发信 
         */


        if (ret == 2) {
            move(1, 0);
            prints("你的信箱容量超出上限, 无法发送信件。", sum, sumlimit);
            pressreturn();
            return -2;
        }
    }
#ifdef INTERNET_PRIVATE_EMAIL
    /*
     * I hate go to , but I use it again for the noodle code :-) 
     */
    else {
        /*
         * if(!strstr(userid,"edu.tw")){
         * if(strstr(userid,"@bbs.ee.nthu."))
         * strcat(userid,"edu.tw");
         * else
         * strcat(userid,".edu.tw");} 
         */
        if (chkusermail(currentuser)) {
            move(1, 0);
            prints("你的信箱容量超出上限, 无法发送信件。", sum, sumlimit);
            pressreturn();
            return -2;
        }
        internet_mail = 1;
        modify_user_mode(IMAIL);
        buf4[0] = ' ';
        sprintf(tmp_fname, "tmp/bbs-internet-gw/%05d", getpid());
        strcpy(filepath, tmp_fname);
        goto edit_mail_file;
    }
    /*
     * end of kludge for internet mail 
     */
#endif

    if (ret == 3)
        return -3;

    setmailpath(filepath, userid);
    if (stat(filepath, &st) == -1) {
        if (mkdir(filepath, 0755) == -1)
            return -1;
    } else {
        if (!(st.st_mode & S_IFDIR))
            return -1;
    }

    memset(&newmessage, 0, sizeof(newmessage));
    now = time(NULL);
    sprintf(fname, "M.%d.A", now);

    setmailfile(filepath, userid, fname);
    ip = strrchr(fname, 'A');
    while ((fp = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
        if (*ip == 'Z')
            ip++, *ip = 'A', *(ip + 1) = '\0';
        else
            (*ip)++;
        setmailfile(filepath, userid, fname);
    }

    close(fp);
    strcpy(newmessage.filename, fname);


    /*
     * strncpy(newmessage.title,title,STRLEN) ; 
     */
    in_mail = true;
#if defined(MAIL_REALNAMES)
    sprintf(genbuf, "%s (%s)", currentuser->userid, currentuser->realname);
#else
/*sprintf(genbuf,"%s (%s)",currentuser->userid,currentuser->username) ;*/
    strcpy(genbuf, currentuser->userid);        /* Leeward 98.04.14 */
#endif
    strncpy(newmessage.owner, genbuf, OWNER_LEN);
	newmessage.owner[OWNER_LEN-1]=0;

    setmailfile(filepath, userid, fname);

#ifdef INTERNET_PRIVATE_EMAIL
  edit_mail_file:
#endif
    if (!title) {
        replymode = 0;
        title = "没主题";
        buf4[0] = '\0';
    } else
        buf4[0] = ' ';

    if (currentuser->signature > numofsig)
        currentuser->signature = 1;
    while (1) {
        sprintf(buf3, "引言模式 [[1m%c[m]", include_mode);
        move(t_lines - 4, 0);
        clrtoeol();
        prints("收信人: [1m%s[m\n", userid);
        clrtoeol();
        prints("使用标题: [1m%-50s[m\n", (title[0] == '\0') ? "[正在设定标题]" : title);
        clrtoeol();
        if (currentuser->signature < 0)
            prints("使用随机签名档     %s", (replymode) ? buf3 : "");
        else
            prints("使用第 [1m%d[m 个签名档     %s", currentuser->signature, (replymode) ? buf3 : "");

        if (buf4[0] == '\0' || buf4[0] == '\n') {
            move(t_lines - 1, 0);
            clrtoeol();
            getdata(t_lines - 1, 0, "标题: ", buf4, 50, DOECHO, NULL, true);
            if ((buf4[0] == '\0' || buf4[0] == '\n')) {
                buf4[0] = ' ';
                continue;
            }
            title = buf4;
            continue;
        }
        move(t_lines - 1, 0);
        clrtoeol();
        /*
         * Leeward 98.09.24 add: viewing signature(s) while setting post head 
         */
        sprintf(buf2, "按 [1;32m0[m~[1;32m%d/V/L[m选/看/随机签名档%s，[1;32mT[m改标题，[1;32mEnter[m接受所有设定: ", numofsig,
                (replymode) ? "，[1;32mY[m/[1;32mN[m/[1;32mR[m/[1;32mA[m改引言模式" : "");
        getdata(t_lines - 1, 0, buf2, ans, 3, DOECHO, NULL, true);
        ans[0] = toupper(ans[0]);       /* Leeward 98.09.24 add; delete below toupper */
        if ((ans[0] - '0') >= 0 && ans[0] - '0' <= 9) {
            if (atoi(ans) <= numofsig)
                currentuser->signature = atoi(ans);
        } else if ((ans[0] == 'Y' || ans[0] == 'N' || ans[0] == 'A' || ans[0] == 'R') && replymode) {
            include_mode = ans[0];
        } else if (ans[0] == 'T') {
            buf4[0] = '\0';
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
            strcpy(newmessage.title, title);
            strncpy(save_title, newmessage.title, STRLEN);
            break;
        }
    }

    do_quote(filepath, include_mode, q_file, quote_user);
    strcpy(quote_title, newmessage.title);

#ifdef INTERNET_PRIVATE_EMAIL
    if (internet_mail) {
        int res, ch;

        if (vedit(filepath, false) == -1) {
            unlink(filepath);
            clear();
            return -2;
        }
        clear();
      redo:
        prints("信件即将寄给 %s \n", userid);
        prints("标题为： %s \n", title);
        prints("确定要寄出吗? (Y/N) [Y]");
        refresh();
        ch = igetkey();
        switch (ch) {
        case KEY_REFRESH:
            move(3, 0);
            goto redo;
        case 'N':
        case 'n':
            prints("%c\n", 'N');
            prints("\n信件已取消...\n");
            res = -2;
            break;
        default:
            {
                /*
                 * uuencode or convert to big5 option -- Add by ming, 96.10.9 
                 */
                char data[3];
                int isuu, isbig5;

                prints("%c\n", 'Y');
                if (askyn("是否备份给自己", false) == true)
                    mail_file(currentuser->userid, tmp_fname, currentuser->userid, save_title, 0);

                prints("若您要转寄的地址无法处理中文请输入 Y 或 y\n");
                getdata(5, 0, "Uuencode? [N]: ", data, 2, DOECHO, 0, 0);
                if (data[0] == 'y' || data[0] == 'Y')
                    isuu = 1;
                else
                    isuu = 0;

                prints("若您要将信件转寄到台湾请输入 Y 或 y\n");
                getdata(7, 0, "转成BIG5码? [N]: ", data, 2, DOECHO, 0, 0);
                if (data[0] == 'y' || data[0] == 'Y')
                    isbig5 = 1;
                else
                    isbig5 = 0;

                getdata(8, 0, "过滤ANSI控制符�? [Y]: ", data, 2, DOECHO, 0, 0);
                if (data[0] == 'n' || data[0] == 'N')
                    noansi = 0;
                else
                    noansi = 1;

                prints("请稍候, 信件传递中...\n");
                refresh();
                /*
                 * res = bbs_sendmail( tmp_fname, title, userid );  
                 */
                res = bbs_sendmail(tmp_fname, title, userid, isuu, isbig5, noansi);

                newbbslog(LOG_USER, "mailed %s", userid);
                break;
            }
        }
        unlink(tmp_fname);
        return res;
    } else
#endif
    {
        if (vedit(filepath, true) == -1) {
            unlink(filepath);
            clear();
            return -2;
        }
        clear();
//        if (askyn("是否备份给自己", false) == true)
//            mail_file(currentuser->userid, filepath, currentuser->userid, save_title, 0);
        /*
         * if(!chkreceiver(userid))
         * {
         * prints("%s 信箱已满,无法收信",userid);
         * return -4;
         * }
         */

        if (false == canIsend2(userid)) {       /* Leeward 98.04.10 */
            prints("[1m[33m很抱歉∶系统无法发出此信．因为 %s 拒绝接收您的信件．[m[m\n\n", userid);
            sprintf(save_title, "退信∶ %s 拒绝接收您的信件．", userid);
            mail_file(currentuser->userid, filepath, currentuser->userid, save_title, 1);
            return -2;
        }
        if (askyn("确定寄出？", true) == false)
            return -2;

        mail_file_sent(userid, filepath, currentuser->userid, save_title, 0);
        setmailfile(genbuf, userid, DOT_DIR);
        if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
            return -1;
        
       if(stat(filepath, &st)!= -1) {
       	user->usedspace+=st.st_size;
       	currentuser->usedspace+=st.st_size;
       }

        newbbslog(LOG_USER, "mailed %s", userid);
        if (!strcasecmp(userid, "SYSOP"))
            updatelastpost("sysmail");
        return 0;
    }
}

int m_send(char userid[])
{
    char uident[STRLEN];

    /*
     * 封禁Mail Bigman:2000.8.22 
     */
    if (HAS_PERM(currentuser, PERM_DENYMAIL))
        return DONOTHING;

    if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND && uinfo.mode != GMENU) {
        move(1, 0);
        clrtoeol();
        modify_user_mode(SMAIL);
        usercomplete("收信人： ", uident);
        if (uident[0] == '\0') {
            clear();
            return 0;
        }
    } else
        strcpy(uident, userid);
    clear();
    switch (do_send(uident, NULL, "")) {
    case -1:
        prints("收信者不正确\n");
        break;
    case -2:
        prints("取消发信\n");
        break;
    case -3:
        prints("'%s' 无法收信\n", uident);
        break;
    case -4:
        clear();
        move(1, 0);
        prints("%s 信箱已满,无法收信\n", uident);
        break;                  /*Haohmaru.4.5.收信限制 */
    case -5:
        clear();
        move(1, 0);
        prints("%s 自杀中，不能收信\n", uident);
        break;                  /*Haohmaru.99.10.26.自杀者不能收信 */
    case -552:
        prints("\n[1m[33m信件超长（本站限定信件长度上限为 %d 字节），取消发信操作[m[m\n", MAXMAILSIZE);
        break;
    default:
        prints("信件已寄出\n");
    }
    pressreturn();
    return 0;
}

int read_mail(fptr)
struct fileheader *fptr;
{
    setmailfile(genbuf, currentuser->userid, fptr->filename);
    ansimore(genbuf, false);
    fptr->accessed[0] |= FILE_READ;
    return 0;
}

int del_mail(int ent, struct fileheader* fh, char* direct)
{
    char buf[PATHLEN];
    char* t;
    struct stat st;

    if(!strcmp(direct, ".DELETED")) {
        strcpy(buf, direct);    
        t = strrchr(buf, '/') + 1;
        strcpy(t, fh->filename);
        if (stat(buf, &st) !=-1) currentuser->usedspace-=st.st_size;
    }
     
    strcpy(buf, direct);
    if ((t = strrchr(buf, '/')) != NULL)
        *t = '\0';
    if (!delete_record(direct, sizeof(*fh), ent, (RECORD_FUNC_ARG) cmpname, fh->filename)) {
        sprintf(genbuf, "%s/%s", buf, fh->filename);
        if(strstr(direct, ".DELETED"))
            unlink(genbuf);
        else {
            strcpy(buf, direct);    
            t = strrchr(buf, '/') + 1;
            strcpy(t, ".DELETED");
            append_record(buf, fh, sizeof(*fh));
        }
        return 0;
    }
    return 1;
}

int mrd;
int delete_new_mail(struct fileheader *fptr, int idc, void *arg)
{
    if (fptr->accessed[1] & FILE_DEL) {
        del_mail(idc, fptr, currmaildir);
        return 1;
    }
    return 0;
}

int read_new_mail(struct fileheader *fptr, int idc, void *arg)
{
    char done = false, delete_it;
    char fname[256];

    if (fptr->accessed[0])
        return 0;
    prints("读取 %s 寄来的 '%s' ?\n", fptr->owner, fptr->title);
    prints("(Yes, or No): ");
    getdata(1, 0, "(Y)读取 (N)不读 (Q)离开 [Y]: ", genbuf, 3, DOECHO, NULL, true);
    if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
        clear();
        return QUIT;
    }
    if (genbuf[0] != 'y' && genbuf[0] != 'Y' && genbuf[0] != '\0') {
        clear();
        return 0;
    }
    read_mail(fptr);
    strcpy(fname, genbuf);
    mrd = 1;
    delete_it = false;
    while (!done) {
        move(t_lines - 1, 0);
        prints("(R)回信, (D)删除, (G)继续 ? [G]: ");
        switch (igetkey()) {
        case 'R':
        case 'r':

            /*
             * 封禁Mail Bigman:2000.8.22 
             */
            if (HAS_PERM(currentuser, PERM_DENYMAIL)) {
                clear();
                move(3, 10);
                prints("很抱歉,您目前没有Mail权限!");
                pressreturn();
                break;
            }
            mail_reply(idc, fptr, currmaildir);
            /*
             * substitute_record(currmaildir, fptr, sizeof(*fptr), dc);
             */
            break;
        case 'D':
        case 'd':
            delete_it = true;
        default:
            done = true;
        }
        if (!done)
            ansimore(fname, false);     /* re-read */
    }
    if (delete_it) {
        clear();
        prints("Delete Message '%s' ", fptr->title);
        getdata(1, 0, "(Yes, or No) [N]: ", genbuf, 3, DOECHO, NULL, true);
        if (genbuf[0] == 'Y' || genbuf[0] == 'y') {     /* if not yes quit */
            fptr->accessed[1] |= FILE_DEL;
        }
    }
    if (substitute_record(currmaildir, fptr, sizeof(*fptr), idc))
        return -1;
    clear();
    return 0;
}

int m_new()
{
    clear();
    mrd = 0;
    modify_user_mode(RMAIL);
    setmailfile(currmaildir, currentuser->userid, ".DIR"); 
    if (apply_record(currmaildir, (APPLY_FUNC_ARG) read_new_mail, sizeof(struct fileheader), NULL, 1, false) == -1) {
        clear();
        move(0, 0);
        prints("No new messages\n\n\n");
        return -1;
    }
    apply_record(currmaildir, (APPLY_FUNC_ARG) delete_new_mail, sizeof(struct fileheader), NULL, 1, true);
/*    	
    if (delcnt) {
        while (delcnt--)
            delete_record(currmaildir, sizeof(struct fileheader), delmsgs[delcnt], NULL, NULL);
    }
*/
    clear();
    move(0, 0);
    if (mrd)
        prints("No more messages.\n\n\n");
    else
        prints("No new messages.\n\n\n");
    return -1;
}

void mailtitle()
{
    /*
     * Leeward 98.01.19 adds below codes for statistics 
     */
#ifdef NINE_BUILD
    int MailSpace = 10000;
#else
    int MailSpace = ((HAS_PERM(currentuser, PERM_SYSOP)
                      || !strcmp(currentuser->userid, "Arbitrator")) ? 9999 : (HAS_PERM(currentuser,
                                                                                        PERM_CHATCLOAK) ? 4000 : (HAS_PERM(currentuser, PERM_MANAGER) ? 600
                                                                                                                  : (HAS_PERM(currentuser, PERM_LOGINOK) ? 240 : 15))));
#endif
    int UsedSpace =get_mailusedspace(currentuser, 0)/1024;

    showtitle("邮件选单    ", BBS_FULL_NAME);
    prints("离开[←,e]  选择[↑,↓]  阅读信件[→,r]  回信[R]  砍信／清除旧信[d,D]  求助[h][m\n");
    /*
     * prints("[44m编号    %-20s %-49s[m\n","发信者","标  题") ; 
     */
    if (0 != get_mailnum() && 0 == UsedSpace)
        UsedSpace = 1;
    else if (UsedSpace < 0)
        UsedSpace = 0;
    prints("[44m编号    %-12s %6s  %-13s您的信箱上限容量%4dK，当前已用%4dK [m\n", (strstr(currmaildir, ".SENT"))?"收信者":"发信者", "日  期", "标  题", MailSpace, UsedSpace);        /* modified by dong , 1998.9.19 */
//    clrtobot();
}

char *maildoent(char *buf, int num, struct fileheader *ent)
{
    time_t filetime;
    char *date;
    char b2[512];
    char status, reply_status;
    char *t;
    extern char ReadPost[];
    extern char ReplyPost[];
    char c1[8];
    char c2[8];
    int same = false;

    filetime = get_posttime(ent); /* 由文件名取得时间 */
    if (filetime > 740000000)
        date = ctime(&filetime) + 4;    /* 时间 -> 英文 */
    else
        /*
         * date = ""; char *类型变量, 可能错误, modified by dong, 1998.9.19 
         */
    {
        date = ctime(&filetime) + 4;
        date = "";
    }

    if (DEFINE(currentuser,DEF_HIGHCOLOR)) {
        strcpy(c1, "[1;33m");
        strcpy(c2, "[1;36m");
    } else {
        strcpy(c1, "[33m");
        strcpy(c2, "[36m");
    }
    if (!strcmp(ReadPost, ent->title) || !strcmp(ReplyPost, ent->title))
        same = true;
    strncpy(b2, ent->owner, OWNER_LEN);
    ent->owner[OWNER_LEN-1]=0;
    if ((t = strchr(b2, ' ')) != NULL)
        *t = '\0';
    if (ent->accessed[0] & FILE_READ) {
        if (ent->accessed[0] & FILE_MARKED)
            status = 'm';
        else
            status = ' ';
    } else {
        if (ent->accessed[0] & FILE_MARKED)
            status = 'M';
        else
            status = 'N';
    }
    if (ent->accessed[0] & FILE_REPLIED) {
        if (ent->accessed[0] & FILE_FORWARDED)
            reply_status = 'A';
        else
            reply_status = 'R';
    } else {
        if (ent->accessed[0] & FILE_FORWARDED)
            reply_status = 'F';
        else
            reply_status = ' ';
    }
    /*
     * if (ent->accessed[0] & FILE_REPLIED)
     * reply_status = 'R';
     * else
     * reply_status = ' '; 
 *//*
 * * added by alex, 96.9.7 
 */
    if (!strncmp("Re:", ent->title, 3)) {
        sprintf(buf, " %s%3d[m %c%c %-12.12s %6.6s  %s%.50s[m", same ? c1 : "", num, reply_status, status, b2, date, same ? c1 : "", ent->title);
    } /* modified by dong, 1998.9.19 */
    else {
        sprintf(buf, " %s%3d[m %c%c %-12.12s %6.6s  ★ %s%.49s[m", same ? c2 : "", num, reply_status, status, b2, date, same ? c2 : "", ent->title);
    }                           /* modified by dong, 1998.9.19 */
    return buf;
}

#ifdef POSTBUG
extern int bug_possible;
#endif

int mail_read(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
    char buf[512], notgenbuf[128];
    char *t;
    int readnext;
    char done = false, delete_it, replied;

    clear();
    readnext = false;
    setqtitle(fileinfo->title);
    strcpy(buf, direct);
    if ((t = strrchr(buf, '/')) != NULL)
        *t = '\0';
    sprintf(notgenbuf, "%s/%s", buf, fileinfo->filename);
    delete_it = replied = false;
    while (!done) {
        ansimore(notgenbuf, false);
        move(t_lines - 1, 0);
        prints("(R)回信, (D)删除, (G)继续? [G]: ");
        switch (igetkey()) {
        case 'R':
        case 'r':

            /*
             * 封禁Mail Bigman:2000.8.22 
             */
            if (HAS_PERM(currentuser, PERM_DENYMAIL)) {
                clear();
                move(3, 10);
                prints("很抱歉,您目前没有Mail权限!");
                pressreturn();
                break;
            }
            replied = true;
            mail_reply(ent, fileinfo, direct);
            break;
        case ' ':
        case 'j':
        case KEY_RIGHT:
        case KEY_DOWN:
        case KEY_PGDN:
            done = true;
            readnext = true;
            break;
        case 'D':
        case 'd':
            delete_it = true;
        default:
            done = true;
        }
    }
    if (delete_it)
        return mail_del(ent, fileinfo, direct);
    else {
        fileinfo->accessed[0] |= FILE_READ;
#ifdef POSTBUG
        if (replied)
            bug_possible = true;
#endif
        substitute_record(currmaildir, fileinfo, sizeof(*fileinfo), ent);
#ifdef POSTBUG
        bug_possible = false;
#endif
    }
    if (readnext == true)
        return READ_NEXT;
    return FULLUPDATE;
}

 /*ARGSUSED*/ static int mail_reply(int ent, struct fileheader *fileinfo, char *direct)
{
    char uid[STRLEN];
    char title[STRLEN];
    char q_file[STRLEN];
    char *t;

    clear();
    modify_user_mode(SMAIL);
    strncpy(uid, fileinfo->owner, OWNER_LEN);
    uid[OWNER_LEN-1]=0;
    if ((t = strchr(uid, ' ')) != NULL)
        *t = '\0';
    if (toupper(fileinfo->title[0]) != 'R' || fileinfo->title[1] != 'e' || fileinfo->title[2] != ':')
        strcpy(title, "Re: ");
    else
        title[0] = '\0';
    strncat(title, fileinfo->title, STRLEN - 5);

    setmailfile(q_file, currentuser->userid, fileinfo->filename);
    strncpy(quote_user, fileinfo->owner, IDLEN);
    quote_user[IDLEN] = 0;
    switch (do_send(uid, title, q_file)) {
    case -1:
        prints("无法投递\n");
        break;
    case -2:
        prints("取消回信\n");
        break;
    case -3:
        prints("'%s' 无法收信\n", uid);
        break;
    case -4:
        clear();
        move(1, 0);
        prints("%s 信箱已满,无法收信\n", uid);
        break;                  /*Haohmaru.4.5.收信限制 */
    case -5:
        clear();
        move(1, 0);
        prints("%s 自杀中，不能收信\n", uid);
        break;                  /*Haohmaru.99.10.26.自杀者不能收信 */
    default:
        prints("信件已寄出\n");
        fileinfo->accessed[0] |= FILE_REPLIED;  /*added by alex, 96.9.7 */
    }
    pressreturn();
    return FULLUPDATE;
}

static int mail_del(int ent, struct fileheader *fileinfo, char *direct)
{
    char buf[512];
    char *t;

    clear();
    prints("删除此信件 '%s' ", fileinfo->title);
    getdata(1, 0, "(Yes, or No) [N]: ", genbuf, 2, DOECHO, NULL, true);
    if (genbuf[0] != 'Y' && genbuf[0] != 'y') { /* if not yes quit */
        move(2, 0);
        prints("取消删除\n");
        pressreturn();
        clear();
        return FULLUPDATE;
    }

    if (del_mail(ent, fileinfo, direct)==0) {
        return DIRCHANGED;
    }
    move(2, 0);
    prints("删除失败\n");
    pressreturn();
    clear();
    return FULLUPDATE;
}

/** Added by netty to handle mail to 0Announce */
int mail_to_tmp(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
    char buf[STRLEN];
    char *p;
    char fname[STRLEN];
    char board[STRLEN];
    char ans[STRLEN];

    if (!HAS_PERM(currentuser, PERM_BOARDS)) {
        return DONOTHING;
    }
    strncpy(buf, direct, sizeof(buf));
    if ((p = strrchr(buf, '/')) != NULL)
        *p = '\0';
    clear();
    sprintf(fname, "%s/%s", buf, fileinfo->filename);
    sprintf(genbuf, "将--%s--存入暂存档,确定吗?(Y/N) [N]: ", fileinfo->title);
    a_prompt(-1, genbuf, ans);
    if (ans[0] == 'Y' || ans[0] == 'y') {
        sprintf(board, "tmp/bm.%s", currentuser->userid);
        if (dashf(board)) {
            sprintf(genbuf, "要附加在旧暂存档之後吗?(Y/N) [N]: ");
            a_prompt(-1, genbuf, ans);
            if (ans[0] == 'Y' || ans[0] == 'y') {
                sprintf(genbuf, "/bin/cat %s >> tmp/bm.%s", fname, currentuser->userid);
                system(genbuf);
            } else {
                /*
                 * sprintf( genbuf, "/bin/cp -r %s  tmp/bm.%s", fname , currentuser->userid );
                 */
                sprintf(genbuf, "tmp/bm.%s", currentuser->userid);
                f_cp(fname, genbuf, 0);
            }
        } else {
            sprintf(genbuf, "tmp/bm.%s", currentuser->userid);
            f_cp(fname, genbuf, 0);
        }
        sprintf(genbuf, " 已将该文章存入暂存档, 请按任何键以继续 << ");
        a_prompt(-1, genbuf, ans);
    }
    clear();
    return FULLUPDATE;
}


#ifdef INTERNET_EMAIL
int mail_forward_internal(int ent, struct fileheader *fileinfo, char *direct, int isuu)
{
    char buf[STRLEN];
    char *p;

    if (strcmp("guest", currentuser->userid) == 0) {
        clear();
        move(3, 10);
        prints("很抱歉,想转寄文章请申请正式ID!");
        pressreturn();
        return FULLUPDATE;
    }

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

    if (!HAS_PERM(currentuser, PERM_FORWARD)) {
        return DONOTHING;
    }
    strncpy(buf, direct, sizeof(buf));
    if ((p = strrchr(buf, '/')) != NULL)
        *p = '\0';
    clear();
    switch (doforward(buf, fileinfo, isuu)) {
    case 0:
        prints("文章转寄完成!\n");
        fileinfo->accessed[0] |= FILE_FORWARDED;        /*added by alex, 96.9.7 */
        /*
         * comment out by jjyang for direct mail delivery 
         */
        newbbslog(LOG_USER, "forwarded file to %s", currentuser->email);
        /*
         * comment out by jjyang for direct mail delivery 
         */

        break;
    case -1:
        prints("Forward failed: system error.\n");
        break;
    case -2:
        prints("Forward failed: missing or invalid address.\n");
        break;
    case -552:
        prints
            ("\n[1m[33m信件超长（本站限定信件长度上限为 %d 字节），取消转寄操作[m[m\n\n请告知收信人（也许就是您自己吧:PP）：\n\n*1* 使用 [1m[33mWWW[m[m 方式访问本站，随时可以保存任意长度的文章到自己的计算机；\n*2* 使用 [1m[33mpop3[m[m 方式从本站用户的信箱取信，没有任何长度限制。\n*3* 如果不熟悉本站的 WWW 或 pop3 服务，请阅读 [1m[33mAnnounce[m[m 版有关公告。\n",
             MAXMAILSIZE);
        break;
    default:
        prints("取消转寄...\n");
    }
    pressreturn();
    clear();
    return FULLUPDATE;
}

int mail_uforward(int ent, struct fileheader *fileinfo, char *direct)
{
    return mail_forward_internal(ent, fileinfo, direct, 1);
}

int mail_forward(int ent, struct fileheader *fileinfo, char *direct)
{
    return mail_forward_internal(ent, fileinfo, direct, 0);
}

#endif

int mail_del_range(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	int ret;
    ret= (del_range(ent, fileinfo, direct, 0));       /*Haohmaru.99.5.14.修改一个bug,
                                                         * 否则可能会因为删信件的.tmpfile而错删版面的.tmpfile */
    if(!strcmp(direct, ".DELETED"))
        get_mailusedspace(currentuser,1);
    return ret;
}

int mail_mark(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
    if (fileinfo->accessed[0] & FILE_MARKED)
        fileinfo->accessed[0] &= ~FILE_MARKED;
    else
        fileinfo->accessed[0] |= FILE_MARKED;
    substitute_record(currmaildir, fileinfo, sizeof(*fileinfo), ent);
    return (PARTUPDATE);
}

void load_mail_list();

int mail_move(int ent, struct fileheader* fileinfo, char* direct)
{
    struct _select_item* sel;
    int i,j;
    char buf[PATHLEN];
    char* t;
    char menu_char[3][10]={"I) 收件箱", "J) 垃圾箱", "Q) 退出"};

    load_mail_list();
    if (!mail_list_t) return DONOTHING;
    clear();
    move(5, 3);
    prints("请选择移动到哪个邮箱");
    sel = (struct _select_item*)malloc(sizeof(struct _select_item)*(mail_list_t+4));
    sel[0].x = 3; sel[0].y = 6; sel[0].hotkey = 'I'; sel[0].type = SIT_SELECT; sel[0].data = menu_char[0];
    sel[1].x = 3; sel[1].y = 7; sel[1].hotkey = 'J'; sel[1].type = SIT_SELECT; sel[1].data = menu_char[1];
    for(i=0;i<mail_list_t;i++) {
    	sel[i+2].x=3;
    	sel[i+2].y=i+8;
    	sel[i+2].hotkey=mail_list[i][0];
    	sel[i+2].type=SIT_SELECT;
    	sel[i+2].data=(void*) mail_list[i];
    }
    sel[mail_list_t+2].x = 3; sel[mail_list_t+2].y = mail_list_t+8;
    sel[mail_list_t+2].hotkey = 'Q'; sel[mail_list_t+2].type = SIT_SELECT;
    sel[mail_list_t+2].data = menu_char[2];
    sel[mail_list_t+3].x = -1; sel[mail_list_t+3].y = -1;
    sel[mail_list_t+3].hotkey = -1; sel[mail_list_t+3].type = 0;
    sel[mail_list_t+3].data = NULL;
    i = simple_select_loop(sel, SIF_NUMBERKEY | SIF_SINGLE | SIF_ESCQUIT, 0, 6, NULL)-1;
    if(i>=0&&i<mail_list_t+2){
        strcpy(buf, direct);    
        t = strrchr(buf, '/') + 1;
        *t = '.';
        t++;
        if (i>=2)
            strcpy(t, mail_list[i-2]+30);
        else if (i==0)
            strcpy(t, "DIR");
        else if (i==1)
            strcpy(t, "DELETED");
        if (strcmp(buf, direct))
        if (!delete_record(direct, sizeof(*fileinfo), ent, (RECORD_FUNC_ARG) cmpname, fileinfo->filename)) {
            append_record(buf, fileinfo, sizeof(*fileinfo));
        }
    }
    free(sel);
    return (FULLUPDATE);
}

extern int mailreadhelp();

struct one_key mail_comms[] = {
    {'d', mail_del},
    {'D', mail_del_range},
    {'r', mail_read},
    {'R', mail_reply},
    {'m', mail_mark},
    {'M', mail_move},
    {'i', mail_to_tmp},
#ifdef INTERNET_EMAIL
    {'F', mail_forward},
    {'U', mail_uforward},
#endif
    /*
     * Added by ming, 96.10.9 
     */
    {'a', auth_search_down},
    {'A', auth_search_up},
    {'/', t_search_down},
    {'?', t_search_up},
    {']', thread_down},
    {'[', thread_up},
    {Ctrl('A'), show_author},
    {Ctrl('Q'), show_authorinfo},       /*Haohmaru.98.12.19 */
    {Ctrl('W'), show_authorBM}, /*cityhunter 00.10.18 */
    {Ctrl('N'), SR_first_new},
    {'\\', SR_last},
    {'=', SR_first},
    {Ctrl('C'), do_cross},
    {Ctrl('S'), SR_read},
    {'n', SR_first_new},
    {'p', SR_read},
    {Ctrl('X'), SR_readX},      /* Leeward 98.10.03 */
    {Ctrl('U'), SR_author},
    {Ctrl('H'), SR_authorX},    /* Leeward 98.10.03 */
    {'h', mailreadhelp},
    {Ctrl('J'), mailreadhelp},
    {Ctrl('O'), add_author_friend},
    {'\0', NULL},
};

int m_read()
{
    m_init();
    in_mail = true;
    i_read(RMAIL, currmaildir, mailtitle, (READ_FUNC) maildoent, &mail_comms[0], sizeof(struct fileheader));
    in_mail = false;
    return FULLUPDATE /* 0 */ ;
}

#ifdef INTERNET_EMAIL

#include <netdb.h>
#include <pwd.h>
#include <time.h>
#define BBSMAILDIR "/usr/spool/mqueue"
int invalidaddr(char *addr)
{
    if (*addr == '\0')
        return 1;               /* blank */
    while (*addr) {
        if (!isalnum(*addr) && strchr("[].%!@:-_", *addr) == NULL)
            return 1;
        addr++;
    }
    return 0;
}

void spacestozeros(s)
char *s;
{
    while (*s) {
        if (*s == ' ')
            *s = '0';
        s++;
    }
}

int getqsuffix(s)
char *s;
{
    struct stat stbuf;
    char qbuf[STRLEN], dbuf[STRLEN];
    char c1 = 'A', c2 = 'A';
    int pos = strlen(BBSMAILDIR) + 3;

    sprintf(dbuf, "%s/dfAA%5d", BBSMAILDIR, getpid());
    sprintf(qbuf, "%s/qfAA%5d", BBSMAILDIR, getpid());
    spacestozeros(dbuf);
    spacestozeros(qbuf);
    while (1) {
        if (stat(dbuf, &stbuf) && stat(qbuf, &stbuf))
            break;
        if (c2 == 'Z') {
            c2 = 'A';
            if (c1 == 'Z')
                return -1;
            else
                c1++;
            dbuf[pos] = c1;
            qbuf[pos] = c1;
        } else
            c2++;
        dbuf[pos + 1] = c2;
        qbuf[pos + 1] = c2;
    }
    strcpy(s, &(qbuf[pos]));
    return 0;
}

int g_send()
{
    char uident[13], tmp[3];
    int cnt, i, n, fmode = false;
    char maillists[STRLEN];
    struct userec *lookupuser;

    /*
     * 封禁Mail Bigman:2000.8.22 
     */
    if (HAS_PERM(currentuser, PERM_DENYMAIL))
        return DONOTHING;

    modify_user_mode(SMAIL);
    clear();
    sethomefile(maillists, currentuser->userid, "maillist");
    cnt = listfilecontent(maillists);
    while (1) {
        if (cnt > maxrecp - 10) {
            move(2, 0);
            prints("目前限制寄信给 [1m%d[m 人", maxrecp);
        }
        getdata(0, 0, "(A)增加 (D)删除 (I)引入好友 (C)清除目前名单 (E)放弃 (S)寄出? [S]： ", tmp, 2, DOECHO, NULL, true);
        if (tmp[0] == '\n' || tmp[0] == '\0' || tmp[0] == 's' || tmp[0] == 'S') {
            break;
        }
        if (tmp[0] == 'a' || tmp[0] == 'd' || tmp[0] == 'A' || tmp[0] == 'D') {
            move(1, 0);
            if (tmp[0] == 'a' || tmp[0] == 'A')
                usercomplete("请依次输入使用者代号(只按 ENTER 结束输入): ", uident);
            else
                namecomplete("请依次输入使用者代号(只按 ENTER 结束输入): ", uident);
            move(1, 0);
            clrtoeol();
            if (uident[0] == '\0')
                continue;
            if (!getuser(uident, &lookupuser)) {
                move(2, 0);
                prints("这个使用者代号是错误的.\n");
                continue;
            } else
                strcpy(uident, lookupuser->userid);
        }
        switch (tmp[0]) {
        case 'A':
        case 'a':
            if (!(lookupuser->userlevel & PERM_READMAIL)) {
                move(2, 0);
                prints("信件无法被寄给: [1m%s[m\n", lookupuser->userid);
                break;
            } else if (seek_in_file(maillists, uident)) {
                move(2, 0);
                prints("已经列为收件人之一 \n");
                break;
            }
            addtofile(maillists, uident);
            cnt++;
            break;
        case 'E':
        case 'e':
            cnt = 0;
            break;
        case 'D':
        case 'd':
            {
                if (seek_in_file(maillists, uident)) {
                    del_from_file(maillists, uident);
                    cnt--;
                }
                break;
            }
        case 'I':
        case 'i':
            n = 0;
            clear();
            for (i = cnt; i < maxrecp && n < nf; i++) {
                int key;

                move(2, 0);
                prints("%s\n", getuserid2(topfriend[n].uid));
                move(4, 0);
                clrtoeol();
                move(3, 0);
                n++;
                if (!fmode) {
                    prints("(A)剩下的全部加入 (Y)加入 (N)不加入 (Q)结束? [Y]:");
                    //TODO: add KEY_REFRESH support
                    key = igetkey();
                } else
                    key = 'Y';
                if (key == 'q' || key == 'Q')
                    break;
                if (key == 'A' || key == 'a') {
                    fmode = true;
                    key = 'Y';
                }
                if (key == '\0' || key == '\n' || key == 'y' || key == 'Y' || '\r' == key) {
                    struct userec *lookupuser;
                    char *errstr;
                    char *touserid = getuserid2(topfriend[n - 1].uid);

                    errstr = NULL;
                    if (!touserid) {
                        errstr = "这个使用者代号是错误的.\n";
                    } else {
                        strcpy(uident, getuserid2(topfriend[n - 1].uid));
                        if (!getuser(uident, &lookupuser)) {
                            errstr = "这个使用者代号是错误的.\n";
                        } else if (!(lookupuser->userlevel & PERM_READMAIL)) {
                            errstr = "信件无法被寄给他\n";
                        } else if (seek_in_file(maillists, uident)) {
                            i--;
                            continue;
                        }
                    }
                    if (errstr) {
                        if (fmode != true) {
                            move(4, 0);
                            prints(errstr);
                            pressreturn();
                        }
                        i--;
                        continue;
                    }
                    addtofile(maillists, uident);
                    cnt++;
                }
            }
            fmode = false;
            clear();
            break;
        case 'C':
        case 'c':
            unlink(maillists);
            cnt = 0;
            break;
        }
        if (tmp[0] == 'e' || tmp[0] == 'E')
            break;
        move(5, 0);
        clrtobot();
        if (cnt > maxrecp)
            cnt = maxrecp;
        move(3, 0);
        clrtobot();
        listfilecontent(maillists);
    }
    if (cnt > 0) {
        G_SENDMODE = 2;
        switch (do_gsend(NULL, NULL, cnt)) {
        case -1:
            prints("信件目录错误\n");
            break;
        case -2:
            prints("取消发信\n");
            break;
        case -4:
            prints("信箱已经超出限额\n");
            break;
        default:
            prints("信件已寄出\n");
        }
        G_SENDMODE = 0;
        pressreturn();
    }
    return 0;
}

/*Add by SmallPig*/

static int do_gsend(char *userid[], char *title, int num)
{
    struct stat st;
    char buf2[256], buf3[STRLEN], buf4[STRLEN];
    int replymode = 1;          /* Post New UI */
    char ans[4], include_mode = 'Y';
    char filepath[STRLEN], tmpfile[STRLEN];
    int cnt;
    FILE *mp;
    extern char quote_title[120];

    /*
     * 添加在好友寄信时的发信上限限制 Bigman 2000.12.11 
     */
    if (chkusermail(currentuser)) {
        move(1, 0);
        prints("你的信箱已经超出限额，无法转寄信件。\n");
        pressreturn();
        return -4;
    }

    in_mail = true;
#if defined(MAIL_REALNAMES)
    sprintf(genbuf, "%s (%s)", currentuser->userid, currentuser->realname);
#else
    /*
     * sprintf(genbuf,"%s (%s)",currentuser->userid,currentuser->username) ; 
     */
    strcpy(genbuf, currentuser->userid);        /* Leeward 98.04.14 */
#endif
    move(1, 0);
    clrtoeol();
    if (!title) {
        replymode = 0;
        title = "没主题";
        buf4[0] = '\0';
    } else
        buf4[0] = ' ';

    sprintf(tmpfile, "tmp/bbs-gsend/%05d", getpid());
    /*
     * Leeward 98.01.17 Prompt whom you are writing to 
     * if (1 == G_SENDMODE)
     * strcpy(lookupuser->userid, "好友名单");
     * else if (2 == G_SENDMODE)
     * strcpy(lookupuser->userid, "寄信名单");
     * else
     * strcpy(lookupuser->userid, "多位网友");
     */

    if (currentuser->signature > numofsig)
        currentuser->signature = 1;
    while (1) {
        sprintf(buf3, "引言模式 [[1m%c[m]", include_mode);
        move(t_lines - 3, 0);
        clrtoeol();
        prints("使用标题: [1m%-50s[m\n", (title[0] == '\0') ? "[正在设定标题]" : title);
        clrtoeol();
        if (currentuser->signature < 0)
            prints("使用随机签名档     %s", (replymode) ? buf3 : "");
        else
            prints("使用第 [1m%d[m 个签名档     %s", currentuser->signature, (replymode) ? buf3 : "");

        if (buf4[0] == '\0' || buf4[0] == '\n') {
            move(t_lines - 1, 0);
            clrtoeol();
            getdata(t_lines - 1, 0, "标题: ", buf4, 50, DOECHO, NULL, true);
            if ((buf4[0] == '\0' || buf4[0] == '\n')) {
                buf4[0] = ' ';
                continue;
            }
            title = buf4;
            continue;
        }
        move(t_lines - 1, 0);
        clrtoeol();
        /*
         * Leeward 98.09.24 add: viewing signature(s) while setting post head 
         */
        sprintf(buf2, "按[1;32m0[m~[1;32m%d/V/L[m选/看/随机签名档%s，[1;32mT[m改标题，[1;32mEnter[m接受所有设定: ", numofsig,
                (replymode) ? "，[1;32mY[m/[1;32mN[m/[1;32mR[m/[1;32mA[m改引言模式" : "");
        getdata(t_lines - 1, 0, buf2, ans, 3, DOECHO, NULL, true);
        ans[0] = toupper(ans[0]);       /* Leeward 98.09.24 add; delete below toupper */
        if ((ans[0] - '0') >= 0 && ans[0] - '0' <= 9) {
            if (atoi(ans) <= numofsig)
                currentuser->signature = atoi(ans);
        } else if ((ans[0] == 'Y' || ans[0] == 'N' || ans[0] == 'A' || ans[0] == 'R') && replymode) {
            include_mode = ans[0];
        } else if (ans[0] == 'T') {
            buf4[0] = '\0';
        } else if (ans[0] == 'V') {     /* Leeward 98.09.24 add: viewing signature(s) while setting post head */
            sethomefile(buf2, currentuser->userid, "signatures");
            move(t_lines - 1, 0);
            if (askyn("预设显示前三个签名档, 要显示全部吗", false) == true)
                ansimore(buf2, 0);
            else {
                clear();
                ansimore2(buf2, false, 0, 18);
            }
        } else if (ans[0] == 'L') {
            currentuser->signature = -1;
        } else {
            strncpy(save_title, title, STRLEN);
            break;
        }
    }

    /*
     * Bigman:2000.8.13 群体发信为什么要引用文章呢 
     */
    /*
     * do_quote( tmpfile,include_mode ); 
     */

    strcpy(quote_title, save_title);
    if (vedit(tmpfile, true) == -1) {
        unlink(tmpfile);
        clear();
        return -2;
    }
    clear();
    if (G_SENDMODE == 2) {
        char maillists[STRLEN];

        sethomefile(maillists, currentuser->userid, "maillist");
        if ((mp = fopen(maillists, "r")) == NULL) {
            return -3;
        }
    }
    for (cnt = 0; cnt < num; cnt++) {
        char uid[13];
        char buf[STRLEN];
        struct userec *user;

        if (G_SENDMODE == 1)
            getuserid(uid, topfriend[cnt].uid);
        else if (G_SENDMODE == 2) {
            if (fgets(buf, STRLEN, mp) != NULL) {
                if (strtok(buf, " \n\r\t") != NULL)
                    strcpy(uid, buf);
                else
                    continue;
            } else {
                cnt = num;
                continue;
            }
        } else
            strcpy(uid, userid[cnt]);
        setmailpath(filepath, uid);
        if (stat(filepath, &st) == -1) {
            if (mkdir(filepath, 0755) == -1) {
                if (G_SENDMODE == 2)
                    fclose(mp);
                return -1;
            }
        } else {
            if (!(st.st_mode & S_IFDIR)) {
                if (G_SENDMODE == 2)
                    fclose(mp);
                return -1;
            }
        }

        if (getuser(uid, &user) == 0) {
            prints("找不到用户%s,请按 Enter 键继续向其他人发信...", uid);
            pressreturn();
            clear();
        } else if (user->userlevel & PERM_SUICIDE) {
            prints("%s 自杀中，不能收信，请按 Enter 键继续向其他人发信...", uid);
            pressreturn();
            clear();
        } else if (!(user->userlevel & PERM_READMAIL)) {
            prints("%s 没有收信的权力，不能收信，请按 Enter 键继续向其他人发信...", uid);
            pressreturn();
            clear();
        } else if (chkusermail(user)) { /*Haohamru.99.4.05 */
            prints("%s 信箱已满,无法收信,请按 Enter 键继续向其他人发信...", uid);
            pressreturn();
            clear();
        } else /* 修正好友发信的错误 Bigman 2000.9.8 */ if (false == canIsend2(uid)) {  /* Leeward 98.04.10 */
            char tmp_title[STRLEN], save_title_bak[STRLEN];

            prints("[1m[33m很抱歉∶系统无法向 %s 发出此信．因为 %s 拒绝接收您的信件．\n\n请按 Enter 键继续向其他人发信...[m[m\n\n", uid, uid);
            pressreturn();
            clear();
            strcpy(save_title_bak, save_title);
            sprintf(tmp_title, "退信∶ %s 拒绝接收您的信件．", uid);
            mail_file(currentuser->userid, tmpfile, currentuser->userid, tmp_title, 0);
            strcpy(save_title, save_title_bak);
        } else {
            mail_file(currentuser->userid, tmpfile, uid, save_title, 0);
        }
    }
    mail_file_sent(".group", tmpfile, currentuser->userid, save_title, 0);
    unlink(tmpfile);
    if (G_SENDMODE == 2)
        fclose(mp);
    return 0;
}

/*Add by SmallPig*/
int ov_send()
{
    int all, i;

    /*
     * 封禁Mail Bigman:2000.8.22 
     */
    if (HAS_PERM(currentuser, PERM_DENYMAIL))
        return DONOTHING;

    modify_user_mode(SMAIL);
    move(1, 0);
    clrtobot();
    move(2, 0);
    prints("寄信给好友名单中的人，目前本站限制仅可以寄给 [1m%d[m 位。\n", maxrecp);
    if (nf <= 0) {
        prints("你并没有设定好友。\n");
        pressanykey();
        clear();
        return 0;
    } else {
        prints("名单如下：\n");
    }
    G_SENDMODE = 1;
    all = (nf >= maxrecp) ? maxrecp : nf;
    for (i = 0; i < all; i++) {
        char *userid;

        userid = getuserid2(topfriend[i].uid);
        if (!userid)
            prints("\x1b[1;32m%-12s\x1b[m ", topfriend[i].uid);
        else
            prints("%-12s ", userid);
        if ((i + 1) % 6 == 0)
            prints("\n");
    }
    pressanykey();
    switch (do_gsend(NULL, NULL, all)) {
    case -1:
        prints("信件目录错误\n");
        break;
    case -2:
        prints("信件取消\n");
        break;
    case -4:
        prints("信箱已经超出限额\n");
        break;
    default:
        prints("信件已寄出\n");
    }
    pressreturn();
    G_SENDMODE = 0;
    return 0;
}

int in_group(uident, cnt)
char uident[maxrecp][STRLEN];
int cnt;
{
    int i;

    for (i = 0; i < cnt; i++)
        if (!strcmp(uident[i], uident[cnt])) {
            return i + 1;
        }
    return 0;
}

int doforward(char *direct, struct fileheader *fh, int isuu)
{
    static char address[STRLEN];
    char fname[STRLEN];
    char receiver[STRLEN];
    char title[STRLEN];
    int return_no;
    char tmp_buf[200];
    int y = 5;
    int sum, sumlimit, numlimit;
    int noansi;

    clear();
    if (address[0] == '\0') {
        strncpy(address, currentuser->email, STRLEN);
        if (strstr(currentuser->email, "@bbs.zixia.net") || strstr(currentuser->email, "bbs@smth.org") || strlen(currentuser->email) == 0) {
            strcpy(address, currentuser->userid);
        }
    }

    if (chkusermail(currentuser)) {
        move(1, 0);
        prints("你的信箱已经超出限额，无法转寄信件。\n");
        pressreturn();
        return -4;
    }

    prints("请直接按 Enter 接受括号内提示的地址, 或者输入其他地址\n");
    prints("(如要转信到自己的BBS信箱,请直接输入你的ID作为地址即可)\n");
    prints("把 %s 的《%s》转寄给:", fh->owner, fh->title);
    sprintf(genbuf, "[%s]: ", address);
    getdata(3, 0, genbuf, receiver, 70, DOECHO, NULL, true);
    if (receiver[0] == '\0') {
        sprintf(genbuf, "确定将文章寄给 %s 吗? (Y/N) [Y]: ", address);
        getdata(3, 0, genbuf, receiver, 3, DOECHO, NULL, true);
        if (receiver[0] == 'n' || receiver[0] == 'N')
            return 1;
        strncpy(receiver, address, STRLEN);
    } else {
        strncpy(address, receiver, STRLEN);
        /*
         * 确认地址是否正确 added by dong, 1998.10.1 
         */
        sprintf(genbuf, "确定将文章寄给 %s 吗? (Y/N) [Y]: ", address);
        getdata(3, 0, genbuf, receiver, 3, DOECHO, NULL, true);
        if (receiver[0] == 'n' || receiver[0] == 'N')
            return 1;
        strncpy(receiver, address, STRLEN);
    }
    if (invalidaddr(receiver))
        return -2;
    if (!HAS_PERM(currentuser, PERM_POST))
        if (!strstr(receiver, "@") && !strstr(receiver, ".")) {
            prints("你尚无权限转寄信件给站内其它用户。");
            pressreturn();
            return -22;
        }

    sprintf(fname, "tmp/forward/%s.%05d", currentuser->userid, getpid());
    /*
     * sprintf( tmp_buf, "cp %s/%s %s",
     * direct, fh->filename, fname);
     */
    sprintf(tmp_buf, "%s/%s", direct, fh->filename);
    f_cp(tmp_buf, fname, 0);
    sprintf(title, "%.50s(转寄)", fh->title);   /*Haohmaru.00.05.01,moved here */
    if (askyn("是否修改文章内容", 0) == 1) {
        vedit(fname, false);
        y = 2;
        newbbslog(LOG_USER, "修改被转贴的文章或信件: %s", title);   /*Haohmaru.00.05.01 */
        /*
         * clear(); 
         */
    }


    {                           /* Leeward 98.04.27: better:-) */

        char *ptrX;

        /*
         * ptrX = strstr(receiver, ".bbs@smth.org");
         * @smth.org @zixia.net 取到前面的用户即可 
         */
        ptrX = strstr(receiver, (const char *) email_domain());

        /*
         * disable by KCN      if (!ptrX) ptrX = strstr(receiver, ".bbs@"); 
         */
        if (ptrX && '@' == *(ptrX - 1))
            *(ptrX - 1) = 0;
    }

    if (!strstr(receiver, "@") && !strstr(receiver, ".")) {     /* sending local file need not uuencode or convert to big5... */
        struct userec *lookupuser;

        prints("转寄信件给 %s, 请稍候....\n", receiver);
        refresh();

        return_no = getuser(receiver, &lookupuser);
        if (return_no == 0) {
            return_no = 1;
            prints("使用者找不到...\n");
        } else {                /* 查完后应该使用lookupuser中的内容,保证大小写正确  period 2000-12-13 */
            strncpy(receiver, lookupuser->userid, IDLEN + 1);
            receiver[IDLEN] = 0;

            /*
             * if(!chkreceiver(receiver,NULL))Haohamru.99.4.05
             * FIXME NULL -> lookupuser，在 zixia.net 上是这么改的... 有没有问题？ 
             */
            if (lookupuser->userlevel & PERM_SUICIDE) {
                prints("%s 自杀中，不能收信\n", receiver);
                return -5;
            }
            if (!(lookupuser->userlevel & PERM_READMAIL)) {
                prints("%s 没有收信的权力，不能收信\n", receiver);
                return -5;
            }


            if (chkusermail(lookupuser)) {      /*Haohamru.99.4.05 */
                prints("%s 信箱已满,无法收信\n", receiver);
                return -4;
            }

            if (false == canIsend2(receiver)) { /* Leeward 98.04.10 */
                prints("[1m[33m很抱歉∶系统无法转寄此信．因为 %s 拒绝接收您的信件．[m[m\n\n", receiver);
                sprintf(title, "退信∶ %s 拒绝接收您的信件．", receiver);
                mail_file(currentuser->userid, fname, currentuser->userid, title, 0);
                return -4;
            }
            return_no = mail_file(currentuser->userid, fname, lookupuser->userid, title, 0);
        }
    } else {
        /*
         * Add by ming, 96.10.9 
         */
        char data[3];
        int isbig5;

        data[0] = 0;
        prints("若您要将信件转寄到台湾请输入 Y 或 y\n");
        getdata(7, 0, "转成BIG5码? [N]: ", data, 2, DOECHO, 0, 0);
        if (data[0] == 'y' || data[0] == 'Y')
            isbig5 = 1;
        else
            isbig5 = 0;

        getdata(8, 0, "过滤ANSI控制符�? [Y]: ", data, 2, DOECHO, 0, 0);
        if (data[0] == 'n' || data[0] == 'N')
            noansi = 0;
        else
            noansi = 1;

        prints("转寄信件给 %s, 请稍候....\n", receiver);
        refresh();

        /*
         * return_no = bbs_sendmail(fname, title, receiver); 
         */

        return_no = bbs_sendmail(fname, title, receiver, isuu, isbig5, noansi);
    }

    unlink(fname);
    return (return_no);
}

#endif

extern int brdnum, yank_flag;
extern struct newpostdata *nbrd;       /*每个版的信息 */

char* NullChar = "";

void add_list(char* s, int (*fptr) ())
{
	struct newpostdata *ptr;
	ptr = &nbrd[brdnum++];
	ptr->name = NullChar;
	ptr->title = s;
	ptr->dir = 3;
	ptr->BM = NullChar;
	ptr->flag = -1;
	ptr->tag = -1;
	ptr->pos = 0;
	ptr->total = 0;
	ptr->unread = 1;
	ptr->fptr = fptr;
	ptr->zap = 0;
}

void add_mlist(char* s, char* t, int k)
{
	struct newpostdata *ptr;
	char buf[PATHLEN], tt[PATHLEN];
	ptr = &nbrd[brdnum++];
	ptr->name = t;
	ptr->title = s;
	ptr->dir = 2;
	ptr->BM = NullChar;
	ptr->flag = -1;
	ptr->tag = k;
	ptr->pos = 0;
	ptr->total = 0;
	ptr->unread = 1;
	ptr->zap = 0;
	sprintf(tt, ".%s", t);
       setmailfile(buf, currentuser->userid, tt);
       ptr->total=getmailnum(buf);
}

char mail_title[9][30]=
{
"N) 览阅新信笺",
"S) 寄信给站上其它使用者",
"I) 收件箱",
"T) 已发送邮件",
"J) 垃圾箱",
"G) 寄给 / 设定寄信名单",
"O)┌设定好友名单",
"F)└寄信给好友名单",
"M) 寄信给所有人"
};

char mail_mtitle[3][10]=
{
"DIR",
"SENT",
"DELETED"
};

extern void load_mail_list();
extern void save_mail_list();
extern int t_override();

int load_mboards()
{
    struct boardheader *bptr;
    struct newpostdata *ptr;
    int n, k;

    brdnum = 0;
    add_list(mail_title[0], m_new);
    add_list(mail_title[1], m_send);
    add_list(mail_title[5], g_send);
    add_list(mail_title[6], t_override);
    add_list(mail_title[7], ov_send);
    if(HAS_PERM(currentuser, PERM_SYSOP))
    	add_list(mail_title[8], mailall);
    add_mlist(mail_title[2], mail_mtitle[0], -1);
    add_mlist(mail_title[3], mail_mtitle[1], -1);
    add_mlist(mail_title[4], mail_mtitle[2], -1);
    for(n=0;n<mail_list_t;n++)
    	add_mlist(mail_list[n], mail_list[n]+30, n);
    
    return 0;
}

int MailProc()
{
    int ifnew = 1, yanksav;

/*    if(heavyload()) ifnew = 0; *//*
 * * no heavyload() in FB2.6x 
 */
    yanksav = yank_flag;
    yank_flag = 3;
    if (!mail_list_t)
        load_mail_list();
    choose_board(ifnew, NULL);
    yank_flag = yanksav;
}

#define BBSMAIN
#include "bbs.h"

#define ROOM_LOCKED 01
#define ROOM_SECRET 02

struct room_struct {
    char name[14];
    char creator[IDLEN+2];
    int style; /* 0 - chat room 1 - killer room */
    unsigned int level;
    int flag;
    int people;
    int maxpeople;
};

#define PEOPLE_SPECTATOR 01
#define PEOPLE_KILLER 02
#define PEOPLE_ALIVE 04
#define PEOPLE_ROOMOP 010

struct people_struct {
    char id[IDLEN+2];
    char nick[NAMELEN];
    int flag;
    int pid;
    int vote;
    int vnum;
};

#define INROOM_STOP 1
#define INROOM_NIGHT 2
#define INROOM_DAY 3

struct inroom_struct {
    char title[NAMELEN];
    int status;
    int killernum;
    struct people_struct peoples[100];
};

struct room_struct * rooms;
int * roomst;
struct inroom_struct inrooms;

char msgs[200][80];
int msgst;

void load_msgs()
{
    FILE* fp;
    int i;
    char filename[80], buf[80];
    msgst=0;
    sprintf(filename, "home/%c/%s/.INROOMMSG%d", toupper(currentuser->userid[0]), currentuser->userid, uinfo.pid);
    fp = fopen(filename, "r");
    if(fp) {
        while(!feof(fp)) {
            if(fgets(buf, 79, fp)==NULL) break;
            if(buf[0]) {
                if(msgst==200) {
                    msgst--;
                    for(i=0;i<msgst;i++)
                        strcpy(msgs[i],msgs[i+1]);
                }
                strcpy(msgs[msgst],buf);
                msgst++;
            }
        }
        fclose(fp);
    }
}

void send_msg(struct people_struct * u, char* msg)
{
    FILE* fp;
    int i;
    char filename[80], buf[80];
    sprintf(filename, "home/%c/%s/.INROOMMSG%d", toupper(u->id[0]), u->id, u->pid);
    fp = fopen(filename, "a");
    if(fp) {
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

int add_room(struct room_struct * r)
{
    int i;
    for(i=0;i<*roomst;i++)
    if(!strcmp(rooms[i].name, r->name))
        return -1;
    memcpy(&(rooms[*roomst]), r, sizeof(struct room_struct));
    (*roomst)++;
    return 0;
}

int del_room(struct room_struct * r)
{
    int i, j;
    for(i=0;i<*roomst;i++)
    if(!strcmp(rooms[i].name, r->name)) {
        (*roomst)--;
        for(j=i;j<*roomst;j++)
            memcpy(&(rooms[i]), &(rooms[i+1]), sizeof(struct room_struct));
        break;
    }
    return 0;
}

void clear_room()
{
    int i;
    for(i=0;i<*roomst;i++)
        if(rooms[i].style==-1) del_room(rooms+i);
}

int load_inroom(struct room_struct * r)
{
    int fd, res=-1;
    struct flock ldata;
    char filename[80];
    sprintf(filename, "home/%c/%s/.INROOM", toupper(r->creator[0]), r->creator);
    if((fd = open(filename, O_RDONLY, 0644))!=-1) {
        ldata.l_type=F_RDLCK;
        ldata.l_whence=0;
        ldata.l_len=0;
        ldata.l_start=0;
        if(fcntl(fd, F_SETLKW, &ldata)!=-1){
            res=0;
            read(fd, &inrooms, sizeof(struct inroom_struct));
            	
            ldata.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &ldata);
        }
        close(fd);
    }
    return res;
}

void clear_inroom(struct room_struct * r)
{
    char filename[80];
    sprintf(filename, "home/%c/%s/.INROOM", toupper(r->creator[0]), r->creator);
    unlink(filename);
}

void save_inroom(struct room_struct * r)
{
    int fd;
    struct flock ldata;
    char filename[80];
    sprintf(filename, "home/%c/%s/.INROOM", toupper(r->creator[0]), r->creator);
    if((fd = open(filename, O_WRONLY|O_CREAT, 0644))!=-1) {
        ldata.l_type=F_WRLCK;
        ldata.l_whence=0;
        ldata.l_len=0;
        ldata.l_start=0;
        if(fcntl(fd, F_SETLKW, &ldata)!=-1){
            write(fd, &inrooms, sizeof(struct inroom_struct));
            	
            ldata.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &ldata);
        }
        close(fd);
    }
}

int change_fd;

void start_change_inroom(struct room_struct * r)
{
    struct flock ldata;
    char filename[80];
    sprintf(filename, "home/%c/%s/.INROOM", toupper(r->creator[0]), r->creator);
    if((change_fd = open(filename, O_RDWR|O_CREAT, 0644))!=-1) {
        ldata.l_type=F_WRLCK;
        ldata.l_whence=0;
        ldata.l_len=0;
        ldata.l_start=0;
        if(fcntl(change_fd, F_SETLKW, &ldata)!=-1){
            lseek(change_fd, 0, SEEK_SET);
            read(change_fd, &inrooms, sizeof(struct inroom_struct));
        }
    }
}

void end_change_inroom()
{
    struct flock ldata;
    ldata.l_whence=0;
    ldata.l_len=0;
    ldata.l_start=0;
    if(change_fd==-1) return;
    lseek(change_fd, 0, SEEK_SET);
    write(change_fd, &inrooms, sizeof(struct inroom_struct));
    	
    ldata.l_type = F_UNLCK;
    fcntl(change_fd, F_SETLKW, &ldata);
    close(change_fd);
}

int can_see(struct room_struct * r)
{
    if(r->level&currentuser->userlevel!=r->level) return 0;
    if(r->style!=1) return 0;
    if(r->flag&ROOM_SECRET&&!HAS_PERM(currentuser, PERM_SYSOP)) return 0;
    return 1;
}

int can_enter(struct room_struct * r)
{
    if(r->level&currentuser->userlevel!=r->level) return 0;
    if(r->style!=1) return 0;
    if(r->flag&ROOM_LOCKED&&!HAS_PERM(currentuser, PERM_SYSOP)) return 0;
    return 1;
}

int room_count()
{
    int i,j=0;
    for(i=0;i<*roomst;i++)
        if(can_see(rooms+i)) j++;
    return j;
}

struct room_struct * room_get(int w)
{
    int i,j=0;
    for(i=0;i<*roomst;i++) {
        if(can_see(rooms+i)) {
            if(w==j) return rooms+i;
            j++;
        }
    }
    return NULL;
}

struct room_struct * find_room(char * s)
{
    int i;
    struct room_struct * r2;
    for(i=0;i<*roomst;i++) {
        r2 = rooms+i;
        if(!can_enter(r2)) continue;
        if(!strcmp(r2->name, s))
            return r2;
    }
    return NULL;
}

struct room_struct * myroom;
int selected = 0, ipage=0, kicked=0, jpage=0;

void refreshit()
{
    int i,j,me;
    for(i=0;i<t_lines-1;i++) {
        move(i, 0);
        clrtoeol();
    }
    move(0,0);
    prints("[44;33;1m ����:[36m%-12s[33m����:[36m%-40s[33m״̬:[36m%6s",
        myroom->name, inrooms.title, (inrooms.status==INROOM_STOP?"δ��ʼ":(inrooms.status==INROOM_NIGHT?"��ҹ��":"�����")));
    clrtoeol();
    resetcolor();
    setfcolor(YELLOW, 1);
    move(1,0);
    prints("�q���������������r�q�����������������������������������������������������������r");
    move(t_lines-2,0);
    prints("�t���������������s�t�����������������������������������������������������������s");
    for(i=2;i<=t_lines-3;i++) {
        move(i,0); prints("��");
        move(i,16); prints("��");
        move(i,18); prints("��");
        move(i,78); prints("��");
    }
    for(me=0;me<myroom->people;me++)
        if(inrooms.peoples[me].pid == uinfo.pid) break;
    for(i=2;i<=t_lines-3;i++) 
    if(ipage+i-2>=0&&ipage+i-2<myroom->people) {
        j=ipage+i-2;
        if(inrooms.status!=INROOM_STOP)
        if(inrooms.peoples[j].flag&PEOPLE_KILLER && (inrooms.peoples[me].flag&PEOPLE_KILLER ||
            inrooms.peoples[me].flag&PEOPLE_SPECTATOR ||
            !(inrooms.peoples[j].flag&PEOPLE_ALIVE))) {
            resetcolor();
            move(i,2);
            setfcolor(RED, 1);
            prints("*");
        }
        if(inrooms.status!=INROOM_STOP)
        if(!(inrooms.peoples[j].flag&PEOPLE_ALIVE)) {
            resetcolor();
            move(i,3);
            setfcolor(BLUE, 1);
            prints("X");
        }
        resetcolor();
        move(i,4);
        if(j==selected) {
            setfcolor(RED, 1);
        }
        if(inrooms.peoples[j].nick[0])
            prints(inrooms.peoples[j].nick);
        else
            prints(inrooms.peoples[j].id);
    }
    resetcolor();
    for(i=2;i<=t_lines-3;i++) 
    if(msgst-1-(t_lines-3-i)-jpage>=0)
    {
        move(i,20);
        prints(msgs[msgst-1-(t_lines-3-i)-jpage]);
    }
}

void room_refresh(int signo)
{
    int y,x;
    signal(SIGUSR1, room_refresh);

    if(load_inroom(myroom)==-1) {
        kicked = 1;
        return;
    }
    load_msgs();
    getyx(&y, &x);
    refreshit();
    move(y, x);
    refresh();
}

void start_game()
{
    int i,j,totalk=0,total=0, me;
    char buf[80];
    start_change_inroom(myroom);
    for(me=0;me<myroom->people;me++)
        if(inrooms.peoples[me].pid==uinfo.pid) break;
    for(i=0;i<myroom->people;i++) {
        inrooms.peoples[i].flag &= ~PEOPLE_KILLER;
    }
    totalk=inrooms.killernum;
    for(i=0;i<myroom->people;i++)
        if(!(inrooms.peoples[i].flag&PEOPLE_SPECTATOR)) 
            total++;
    if(total<6) {
        send_msg(inrooms.peoples+me, "\x1b[31;1m����6�˲μӲ��ܿ�ʼ��Ϸ\x1b[m");
        end_change_inroom();
        kill(inrooms.peoples[me].pid, SIGUSR1);
        return;
    }
    if(totalk==0) totalk=((double)total/6+0.5);
    if(totalk>total) {
        send_msg(inrooms.peoples+me, "\x1b[31;1m����������Ҫ��Ļ�������,�޷���ʼ��Ϸ\x1b[m");
        end_change_inroom();
        kill(inrooms.peoples[me].pid, SIGUSR1);
        return;
    }
    inrooms.status = INROOM_NIGHT;
    sprintf(buf, "\x1b[31;1m��Ϸ��ʼ��!\x1b[m");
    for(i=0;i<myroom->people;i++)
        send_msg(inrooms.peoples+i, buf);
    for(i=0;i<totalk;i++) {
        do{
            j=rand()%myroom->people;
        }while(inrooms.peoples[j].flag!=0);
        inrooms.peoples[j].flag = PEOPLE_KILLER;
        send_msg(inrooms.peoples+j, "������һ���޳ܵĻ���\n����ļ⵶(\x1b[31;1mCtrl+S\x1b[m)ѡ����Ҫ�к����˰�...");
    }
    for(i=0;i<myroom->people;i++) 
    if(!(inrooms.peoples[i].flag&PEOPLE_SPECTATOR))
    {
        inrooms.peoples[i].flag |= PEOPLE_ALIVE;
        if(!(inrooms.peoples[i].flag&PEOPLE_KILLER))
            send_msg(inrooms.peoples+i, "����������...");
    }
    end_change_inroom();
    for(i=0;i<myroom->people;i++)
        kill(inrooms.peoples[i].pid, SIGUSR1);
}

#define menust 8
int do_com_menu()
{
    char menus[menust][15]=
        {"0-����","1-�˳���Ϸ","2-������", "3-����б�", "4-�Ļ���", "5-���÷���", "6-�����", "7-��ʼ��Ϸ"};
    int menupos[menust],i,j,sel=0,ch,max=0,me;
    char buf[80];
    menupos[0]=0;
    for(i=1;i<menust;i++)
        menupos[i]=menupos[i-1]+strlen(menus[i-1])+1;
    do{
        resetcolor();
        move(t_lines-1,0);
        clrtoeol();
        for(j=0;j<myroom->people;j++)
            if(inrooms.peoples[j].pid == uinfo.pid) break;
        for(i=0;i<menust;i++) 
        if(inrooms.peoples[j].flag&PEOPLE_ROOMOP||i<=3)
        if(i!=7||inrooms.status==INROOM_STOP) {
            resetcolor();
            move(t_lines-1, menupos[i]);
            if(i==sel) {
                setfcolor(RED,1);
            }
            if(i>=max-1) max=i+1;
            prints(menus[i]);
        }
        ch=igetkey();
        switch(ch){
        case KEY_LEFT:
        case KEY_UP:
            sel--;
            if(sel<0) sel=max-1;
            break;
        case KEY_RIGHT:
        case KEY_DOWN:
            sel++;
            if(sel>=max) sel=0;
            break;
        case '\n':
        case '\r':
            switch(sel) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    move(t_lines-1, 0);
                    resetcolor();
                    clrtoeol();
                    getdata(t_lines-1, 0, "����������:", buf, 30, 1, 0, 1);
                    if(buf[0]) {
                        j=0;
                        for(i=0;i<myroom->people;i++)
                            if(!strcmp(buf,inrooms.peoples[i].id) || !strcmp(buf,inrooms.peoples[i].nick)) j=1;
                        if(j) {
                            move(t_lines-1,0);
                            resetcolor();
                            clrtoeol();
                            prints(" �����������������");
                            refresh();
                            sleep(1);
                            return 0;
                        }
                        start_change_inroom(myroom);
                        for(me=0;me<myroom->people;me++)
                            if(inrooms.peoples[me].pid==uinfo.pid) break;
                        strcpy(inrooms.peoples[me].nick, buf);
                        end_change_inroom();
                        for(i=0;i<myroom->people;i++)
                            kill(inrooms.peoples[i].pid, SIGUSR1);
                    }
                    return 0;
                case 7:
                    start_game();
                    return 0;
            }
            break;
        default:
            for(i=0;i<menust;i++)
                if(ch==menus[i][0]) sel=i;
            break;
        }
    }while(1);
}

void join_room(struct room_struct * r)
{
    char buf[80],buf2[80];
    int i,j,killer,me;
    clear();
    sprintf(buf, "home/%c/%s/.INROOMMSG%d", toupper(currentuser->userid[0]), currentuser->userid, uinfo.pid);
    unlink(buf);
    myroom = r;
    signal(SIGUSR1, room_refresh);
    start_change_inroom(r);
    i=r->people;
    inrooms.peoples[i].flag = 0;
    strcpy(inrooms.peoples[i].id, currentuser->userid);
    inrooms.peoples[i].nick[0]=0;
    inrooms.peoples[i].pid = uinfo.pid;
    if(i==0) {
        inrooms.status = INROOM_STOP;
        inrooms.killernum = 0;
        strcpy(inrooms.title, "��ɱ��ɱ��ɱɱɱ");
        inrooms.peoples[i].flag = PEOPLE_ROOMOP;
    }
    r->people++;
    end_change_inroom();

    sprintf(buf, "%s���뷿��", currentuser->userid);
    for(i=0;i<myroom->people;i++) {
        send_msg(inrooms.peoples+i, buf);
        kill(inrooms.peoples[i].pid, SIGUSR1);
    }

    room_refresh(0);
    while(1){
        do{
            int ch;
            ch=-getdata(t_lines-1, 0, "����:", buf, 75, 1, NULL, 1);
            if(kicked) goto quitgame;
            if(ch==KEY_UP) {
                selected--;
                if(selected<0) selected = myroom->people-1;
                if(ipage>selected) ipage=selected;
                refreshit();
            }
            else if(ch==KEY_DOWN) {
                selected++;
                if(selected>=myroom->people) selected=0;
                if(selected>ipage+t_lines-5) ipage=selected-(t_lines-5);
                refreshit();
            }
            else if(ch==KEY_PGUP) {
                jpage+=t_lines/2;
                refreshit();
            }
            else if(ch==KEY_PGDN) {
                jpage-=t_lines/2;
                if(jpage>=0) jpage=0;
                refreshit();
            }
            else if(ch==Ctrl('S')) {
                int pid;
                for(me=0;me<myroom->people;me++)
                    if(inrooms.peoples[me].pid == uinfo.pid) break;
                pid=inrooms.peoples[selected].pid;
                if(inrooms.peoples[me].flag&PEOPLE_ALIVE&&
                    (inrooms.peoples[me].flag&PEOPLE_KILLER&&inrooms.status==INROOM_NIGHT ||
                    inrooms.status==INROOM_DAY)) {
                    if(inrooms.peoples[selected].flag&PEOPLE_ALIVE && 
                        !(inrooms.peoples[selected].flag&PEOPLE_SPECTATOR) &&
                        selected!=me) {
                        int i,j;
                        sprintf(buf, "\x1b[32;1m%sͶ��%sһƱ\x1b[m", inrooms.peoples[me].nick[0]?inrooms.peoples[me].nick:inrooms.peoples[me].id,
                            inrooms.peoples[selected].nick[0]?inrooms.peoples[selected].nick:inrooms.peoples[selected].id);
                        start_change_inroom(myroom);
                        inrooms.peoples[me].vote = pid;
                        end_change_inroom();
                        if(inrooms.status==INROOM_NIGHT) {
                            for(i=0;i<myroom->people;i++)
                                if(inrooms.peoples[i].flag&PEOPLE_KILLER||
                                    inrooms.peoples[i].flag&PEOPLE_SPECTATOR)
                                    send_msg(inrooms.peoples+i, buf);
                        }
                        else {
                            for(i=0;i<myroom->people;i++)
                                send_msg(inrooms.peoples+i, buf);
                        }
                        j=1;
                        for(i=0;i<myroom->people;i++)
                        if(!(inrooms.peoples[i].flag&PEOPLE_SPECTATOR) &&
                            inrooms.peoples[i].flag&PEOPLE_ALIVE &&
                            (inrooms.peoples[i].flag&PEOPLE_KILLER||inrooms.status==INROOM_DAY))
                            if(inrooms.peoples[i].vote == 0) j=0;
                        if(j) {
                            int max=0, ok=0, maxi, maxpid;
                            for(i=0;i<myroom->people;i++)
                                inrooms.peoples[i].vnum = 0;
                            for(i=0;i<myroom->people;i++)
                            if(!(inrooms.peoples[i].flag&PEOPLE_SPECTATOR) &&
                                inrooms.peoples[i].flag&PEOPLE_ALIVE &&
                                (inrooms.peoples[i].flag&PEOPLE_KILLER||inrooms.status==INROOM_DAY)) {
                                for(j=0;j<myroom->people;j++)
                                    if(inrooms.peoples[j].pid == inrooms.peoples[i].vote)
                                        inrooms.peoples[j].vnum++;
                            }
                            sprintf(buf, "ͶƱ���:");
                            if(inrooms.status==INROOM_NIGHT) {
                                for(j=0;j<myroom->people;j++)
                                    if(inrooms.peoples[j].flag&PEOPLE_KILLER||
                                        inrooms.peoples[j].flag&PEOPLE_SPECTATOR)
                                        send_msg(inrooms.peoples+j, buf);
                            }
                            else {
                                for(j=0;j<myroom->people;j++)
                                    send_msg(inrooms.peoples+j, buf);
                            }
                            for(i=0;i<myroom->people;i++)
                            if(!(inrooms.peoples[i].flag&PEOPLE_SPECTATOR) &&
                                inrooms.peoples[i].flag&PEOPLE_ALIVE) {
                                sprintf(buf, "%s��ͶƱ��: %d Ʊ", 
                                    inrooms.peoples[i].nick[0]?inrooms.peoples[i].nick:inrooms.peoples[i].id,
                                    inrooms.peoples[i].vnum);
                                if(inrooms.peoples[i].vnum==max)
                                    ok=0;
                                if(inrooms.peoples[i].vnum>max) {
                                    max=inrooms.peoples[i].vnum;
                                    ok=1;
                                    maxi=i;
                                    maxpid=inrooms.peoples[i].pid;
                                }
                                if(inrooms.status==INROOM_NIGHT) {
                                    for(j=0;j<myroom->people;j++)
                                        if(inrooms.peoples[j].flag&PEOPLE_KILLER||
                                            inrooms.peoples[j].flag&PEOPLE_SPECTATOR)
                                            send_msg(inrooms.peoples+j, buf);
                                }
                                else {
                                    for(j=0;j<myroom->people;j++)
                                        send_msg(inrooms.peoples+j, buf);
                                }
                            }
                            if(!ok) {
                                sprintf(buf, "���Ʊ����ͬ,������Э�̽��...");
                                if(inrooms.status==INROOM_NIGHT) {
                                    for(j=0;j<myroom->people;j++)
                                        if(inrooms.peoples[j].flag&PEOPLE_KILLER||
                                            inrooms.peoples[j].flag&PEOPLE_SPECTATOR)
                                            send_msg(inrooms.peoples+j, buf);
                                }
                                else {
                                    for(j=0;j<myroom->people;j++)
                                        send_msg(inrooms.peoples+j, buf);
                                }
                                start_change_inroom(myroom);
                                for(j=0;j<myroom->people;j++)
                                    inrooms.peoples[j].vote=0;
                                end_change_inroom();
                            }
                            else {
                                int a=0,b=0;
                                if(inrooms.status == INROOM_DAY)
                                    sprintf(buf, "���Ҵ�����!");
                                else
                                    sprintf(buf, "�㱻����ɱ����!");
                                send_msg(inrooms.peoples+maxi, buf);
                                if(inrooms.status == INROOM_DAY) {
                                    if(inrooms.peoples[maxi].flag&PEOPLE_KILLER)
                                        sprintf(buf, "����%s��������!",
                                            inrooms.peoples[maxi].nick[0]?inrooms.peoples[maxi].nick:inrooms.peoples[maxi].id);
                                    else
                                        sprintf(buf, "����%s��������!",
                                            inrooms.peoples[maxi].nick[0]?inrooms.peoples[maxi].nick:inrooms.peoples[maxi].id);
                                }
                                else
                                    sprintf(buf, "%s��ɱ����!",
                                        inrooms.peoples[maxi].nick[0]?inrooms.peoples[maxi].nick:inrooms.peoples[maxi].id);
                                for(j=0;j<myroom->people;j++)
                                    if(j!=maxi)
                                        send_msg(inrooms.peoples+j, buf);
                                start_change_inroom(myroom);
                                for(i=0;i<myroom->people;i++)
                                    if(inrooms.peoples[i].pid == maxpid)
                                        inrooms.peoples[i].flag &= ~PEOPLE_ALIVE;
                                for(i=0;i<myroom->people;i++)
                                    if(inrooms.peoples[i].flag&PEOPLE_ALIVE) {
                                        if(inrooms.peoples[i].flag&PEOPLE_KILLER) a++;
                                        else b++;
                                    }
                                if(a>=b-1) {
                                    inrooms.status = INROOM_STOP;
                                    for(i=0;i<myroom->people;i++) {
                                        send_msg(inrooms.peoples+i, "���˻����ʤ��...");
                                        for(j=0;j<myroom->people;j++)
                                        if(inrooms.peoples[j].flag&PEOPLE_KILLER &&
                                            inrooms.peoples[j].flag&PEOPLE_ALIVE) {
                                            sprintf(buf, "ԭ��%s�ǻ���!",
                                                inrooms.peoples[j].nick[0]?inrooms.peoples[j].nick:inrooms.peoples[j].id);
                                            send_msg(inrooms.peoples+i, buf);
                                        }
                                    }
                                }
                                else if(a==0) {
                                    inrooms.status = INROOM_STOP;
                                    for(i=0;i<myroom->people;i++)
                                        send_msg(inrooms.peoples+i, "���л��˶��������ˣ����˻����ʤ��...");
                                }
                                else if(inrooms.status == INROOM_DAY) {
                                    inrooms.status = INROOM_NIGHT;
                                    for(i=0;i<myroom->people;i++)
                                        send_msg(inrooms.peoples+i, "�ֲ���ҹɫ�ֽ�����...");
                                }
                                else {
                                    inrooms.status = INROOM_DAY;
                                    for(i=0;i<myroom->people;i++)
                                        send_msg(inrooms.peoples+i, "������...");
                                }
                                for(i=0;i<myroom->people;i++)
                                    inrooms.peoples[i].vote = 0;
                                end_change_inroom();
                            }
                        }
                        for(i=0;i<myroom->people;i++)
                            kill(inrooms.peoples[i].pid, SIGUSR1);
                    }
                    else {
                        if(selected==me)
                            send_msg(inrooms.peoples+me, "\x1b[31;1m�㲻��ѡ����ɱ\x1b[m");
                        else if(!(inrooms.peoples[selected].flag&PEOPLE_ALIVE))
                            send_msg(inrooms.peoples+me, "\x1b[31;1m��������\x1b[m");
                        else
                            send_msg(inrooms.peoples+me, "\x1b[31;1m�������Թ���\x1b[m");
                        kill(inrooms.peoples[me].pid, SIGUSR1);
                    }
                }
            }
            else if(!buf[0]) {
                if(do_com_menu()) goto quitgame;
            }
            else {
                break;
            }
        }while(1);
        for(me=0;me<myroom->people;me++)
            if(inrooms.peoples[me].pid == uinfo.pid) break;
        strcpy(buf2, buf);
        sprintf(buf, "%s: %s", 
            inrooms.peoples[me].nick[0]?inrooms.peoples[me].nick:inrooms.peoples[me].id, 
            buf2);
        if(inrooms.status==INROOM_NIGHT) {
            for(me=0;me<myroom->people;me++)
                if(inrooms.peoples[me].pid == uinfo.pid) break;
            if(inrooms.peoples[me].flag&PEOPLE_KILLER)
            for(i=0;i<myroom->people;i++) {
                if(inrooms.peoples[i].flag&PEOPLE_KILLER||
                    inrooms.peoples[i].flag&PEOPLE_SPECTATOR) {
                    send_msg(inrooms.peoples+i, buf);
                    kill(inrooms.peoples[i].pid, SIGUSR1);
                }
            }
        }
        else {
            for(me=0;me<myroom->people;me++)
                if(inrooms.peoples[me].pid == uinfo.pid) break;
            if(!(inrooms.peoples[me].flag&PEOPLE_SPECTATOR))
            for(i=0;i<myroom->people;i++) {
                send_msg(inrooms.peoples+i, buf);
                kill(inrooms.peoples[i].pid, SIGUSR1);
            }
        }
    }

quitgame:
    killer=0;
    start_change_inroom(r);
    if(change_fd!=-1) {
        for(me=0;me<myroom->people;me++)
            if(inrooms.peoples[me].pid == uinfo.pid) break;
        if(inrooms.peoples[me].flag&PEOPLE_ROOMOP) {
            end_change_inroom();
            clear_inroom(myroom);
            for(i=0;i<myroom->people;i++)
                kill(inrooms.peoples[i].pid, SIGUSR1);
            myroom->people = 0;
            myroom->style = -1;
            goto quitgame2;
        }
        for(i=0;i<myroom->people;i++)
            if(inrooms.peoples[i].pid==uinfo.pid) {
                if(inrooms.peoples[i].flag&PEOPLE_KILLER) killer=1;
                strcpy(buf2, inrooms.peoples[i].nick);
                if(!buf2[0])
                    strcpy(buf2, inrooms.peoples[i].id);
                for(j=i;j<myroom->people-1;j++)
                    memcpy(inrooms.peoples+j, inrooms.peoples+j+1, sizeof(struct people_struct));
                break;
            }
        r->people--;
    }
    end_change_inroom();

    if(killer)
        sprintf(buf, "ɱ��%sǱ����", buf2);
    else
        sprintf(buf, "%s�뿪����", buf2);
    for(i=0;i<myroom->people;i++) {
        send_msg(inrooms.peoples+i, buf);
        kill(inrooms.peoples[i].pid, SIGUSR1);
    }
quitgame2:
    
    signal(SIGUSR1, talk_request);
}

static int room_list_refresh(struct _select_def *conf)
{
    clear();
    docmdtitle("[��Ϸ��ѡ��]",
              "  �˳�[\x1b[1;32m��\x1b[0;37m,\x1b[1;32me\x1b[0;37m] ����[\x1b[1;32mEnter\x1b[0;37m] ѡ��[\x1b[1;32m��\x1b[0;37m,\x1b[1;32m��\x1b[0;37m] ����[\x1b[1;32ma\x1b[0;37m] ����[\x1b[1;32mJ\x1b[0;37m] \x1b[m");
    move(2, 0);
    prints("[0;1;37;44m    %4s %-40s %-12s %4s %4s", "���", "��Ϸ������", "������", "����", "����");
    clrtoeol();
    resetcolor();
    update_endline();
    return SHOW_CONTINUE;
}

static int room_list_show(struct _select_def *conf, int i)
{
    struct room_struct * r = room_get(i-1);
    if(r)
        prints("  %3d  %-40s %-12s %3d  %4s", i, r->name, r->creator, r->people, "ɱ��");
    return SHOW_CONTINUE;
}

static int room_list_select(struct _select_def *conf)
{
    struct room_struct * r = room_get(conf->pos-1), * r2;
    if(r==NULL) return SHOW_CONTINUE;
    if((r2=find_room(r->name))==NULL) {
        move(0, 0);
        clrtoeol();
        prints(" �÷����ѱ�����!");
        refresh(); sleep(1);
        return SHOW_REFRESH;
    }
    if(r2->people==r2->maxpeople) {
        move(0, 0);
        clrtoeol();
        prints(" �÷�����������");
        refresh(); sleep(1);
        return SHOW_REFRESH;
    }
    join_room(find_room(r2->name));
    return SHOW_DIRCHANGE;
}

static int room_list_getdata(struct _select_def *conf, int pos, int len)
{
    conf->item_count = room_count();
    return SHOW_CONTINUE;
}

static int room_list_prekey(struct _select_def *conf, int *key)
{
    switch (*key) {
    case 'e':
    case 'q':
        *key = KEY_LEFT;
        break;
    case 'p':
    case 'k':
        *key = KEY_UP;
        break;
    case ' ':
    case 'N':
        *key = KEY_PGDN;
        break;
    case 'n':
    case 'j':
        *key = KEY_DOWN;
        break;
    }
    return SHOW_CONTINUE;
}

static int room_list_key(struct _select_def *conf, int key)
{
    struct room_struct r, *r2;
    char name[40];
    switch(key) {
    case 'a':
        strcpy(r.creator, currentuser->userid);
        getdata(0, 0, "������:", name, 12, 1, NULL, 1);
        if(!name[0]) return SHOW_REFRESH;
        strcpy(r.name, name);
        r.style = 1;
        r.flag = 0;
        r.people = 0;
        r.maxpeople = 100;
        if(add_room(&r)==-1) {
            move(0, 0);
            clrtoeol();
            prints(" ��һ�����ֵķ�����!");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        clear_inroom(&r);
        join_room(find_room(r.name));
        return SHOW_DIRCHANGE;
    case 'J':
        getdata(0, 0, "������:", name, 12, 1, NULL, 1);
        if(!name[0]) return SHOW_REFRESH;
        if((r2=find_room(name))==NULL) {
            move(0, 0);
            clrtoeol();
            prints(" û���ҵ��÷���!");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        if(r2->people==r2->maxpeople) {
            move(0, 0);
            clrtoeol();
            prints(" �÷�����������");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        join_room(find_room(name));
        return SHOW_DIRCHANGE;
    }
    return SHOW_CONTINUE;
}

int choose_room()
{
    struct _select_def grouplist_conf;
    int i;
    POINT *pts;

    clear_room();
    bzero(&grouplist_conf, sizeof(struct _select_def));
    grouplist_conf.item_count = room_count();
    if (grouplist_conf.item_count == 0) {
        grouplist_conf.item_count = 1;
    }
    pts = (POINT *) malloc(sizeof(POINT) * BBS_PAGESIZE);
    for (i = 0; i < BBS_PAGESIZE; i++) {
        pts[i].x = 2;
        pts[i].y = i + 3;
    }
    grouplist_conf.item_per_page = BBS_PAGESIZE;
    grouplist_conf.flag = LF_VSCROLL | LF_BELL | LF_LOOP | LF_MULTIPAGE;
    grouplist_conf.prompt = "��";
    grouplist_conf.item_pos = pts;
    grouplist_conf.arg = NULL;
    grouplist_conf.title_pos.x = 0;
    grouplist_conf.title_pos.y = 0;
    grouplist_conf.pos = 1;     /* initialize cursor on the first mailgroup */
    grouplist_conf.page_pos = 1;        /* initialize page to the first one */

    grouplist_conf.on_select = room_list_select;
    grouplist_conf.show_data = room_list_show;
    grouplist_conf.pre_key_command = room_list_prekey;
    grouplist_conf.key_command = room_list_key;
    grouplist_conf.show_title = room_list_refresh;
    grouplist_conf.get_data = room_list_getdata;
    list_select_loop(&grouplist_conf);
    free(pts);
}

int killer_main()
{
    int i,oldmode;
    void * shm;
    shm=attach_shm("KILLER_SHMKEY", 3451, sizeof(struct room_struct)*1000+4, &i);
    rooms = shm+4;
    roomst = shm;
    if(i) (*roomst) = 0;
    oldmode = uinfo.mode;
    modify_user_mode(KILLER);
    choose_room();
    modify_user_mode(oldmode);
}


#include "bbs.h"

struct userec deliveruser;

int main(int argc, char **argv)
{
    FILE *fp, *fp2;
    char fn[80],fn2[80];
    char * p;
    int i,j,ip[4],t,k;
    time_t tt;
    chdir(BBSHOME);
    resolve_boards();
    resolve_ucache();
    strcpy(fn, ".IPdenys");
    strcpy(fn2, "tmp/showipdeny.txt");
    fp=fopen(fn, "r");
    if(fp) {
        fp2=fopen(fn2, "w");
        while(!feof(fp)) {
            if(fscanf(fp, "%d %ld %d.%d.%d.%d %d", &i, &j, &ip[0], &ip[1], &ip[2], &ip[3], &t)<=0) break;
            tt=(time_t) j;
            p = ctime(&tt);
            p[19]=0; p+=4;
            if(i==0)
                fprintf(fp2, "%s 来自 %d.%d.%d.%d 两次连接时间太短.一小时内共连接%d次.\n", p, ip[0],ip[1],ip[2],ip[3],t+1);
            else
                fprintf(fp2, "%s 来自 %d.%d.%d.%d 连接过于频繁.    一小时内共连接%d次.\n", p, ip[0],ip[1],ip[2],ip[3],t);
        }
        fclose(fp);
        fclose(fp2);
        unlink(fn);
        bzero(&deliveruser, sizeof(struct userec));
        strcpy(deliveruser.userid, "deliver");
        deliveruser.userlevel = -1;
        strcpy(deliveruser.username, "自动发信系统");
        currentuser = &deliveruser;
        strcpy(fromhost, "天堂");
        tt = time(0);
        p = ctime(&tt);
        p[19]=0; p+=4;
        sprintf(fn, "%s的犯罪记录", p);
        post_file(&deliveruser, NULL, fn2, "system_dev", fn, 0, 1);
    }
    return 0;
}

#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#define IDEINFO 300
#define FILESTAT 301
#define INODEINFO 302
#define PID_BOTOM 303
#define PID_TOP 400
#define NAME_BOTOM 401
#define NAME_TOP 500
#define STATUS_BOTOM 501
#define STATUS_TOP 600


int
procfsisdir(struct inode *ip) {
    if(ip->major == PROCFS && ip->type == T_DEV){
        if((ip->minor == 0) || (ip->minor == 1 && ip->inum >= PID_BOTOM && ip->inum <PID_TOP) || (ip->minor == 2 && ip->inum > STATUS_TOP )) //it's a directory
            return 1;
    }
    return 0; //not a directory
}

void
procfsiread(struct inode *dp, struct inode *ip) {
    ip->ref=1;
    ip->type=T_DEV;
    ip->major=PROCFS;
    ip->nlink=1;
    ip->valid=1;
    // check if file or is a fake directory
    int inum = ip->inum;
    if (inum < IDEINFO) // proc
        ip->minor = 0;
    else if (inum >= IDEINFO && inum < PID_TOP)
        ip->minor = 1;
    else if (inum > PID_TOP)
        ip->minor = 2;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
    char ansBuffer[sizeof(struct dirent) * NPROC + + sizeof(struct dirent) * NINODE + 1000];
    int ansLength = 0;
    if (ip->minor == 0){
        cprintf("proc\n");
        struct dirent currDirent;
        struct dirent rootDirent;
        struct dirent ideinfoDirent;
        struct dirent filestatDirent;
        struct dirent inodeinfoDirent;
        //struct dirent pidDirent;

        currDirent.inum = ip->inum;
        strncpy(currDirent.name,".",sizeof("."));
        memmove(ansBuffer,(char*)&currDirent,sizeof(currDirent));

        rootDirent.inum = ROOTINO;
        strncpy(rootDirent.name,"..",sizeof(".."));
        memmove(ansBuffer+sizeof(rootDirent),(char*)&rootDirent,sizeof(rootDirent));

        ideinfoDirent.inum = IDEINFO;
        strncpy(ideinfoDirent.name, "ideinfo", sizeof("ideinfo"));
        memmove(ansBuffer + sizeof(struct dirent) * 2, (char*)&ideinfoDirent, sizeof(ideinfoDirent));

        filestatDirent.inum = FILESTAT;
        strncpy(filestatDirent.name, "filestat", sizeof("filestat"));
        memmove(ansBuffer + sizeof(struct dirent) * 3, (char*)&filestatDirent, sizeof(filestatDirent));

        inodeinfoDirent.inum = INODEINFO;
        strncpy(inodeinfoDirent.name, "inodeinfo", sizeof("inodeinfo"));
        memmove(ansBuffer + sizeof(struct dirent) * 4, (char*)&inodeinfoDirent, sizeof(inodeinfoDirent));
        int counter = 5;
        /*
        int i = 0;

        for(p = get_ptable(); p < &get_ptable()[NPROC]; p++){
            if(p->state != UNUSED && p->state != ZOMBIE){
                pidDirent.inum = 403 + i;
                itoa(pidDirent.name , p->pid);
                memmove(ansBuffer + sizeof(struct dirent) * counter, (char*)&pidDirent, sizeof(pidDirent));
                counter++;
            }
            i++;
        }
         */
        ansLength = counter * sizeof(struct dirent);
    }

        //ansLength = Proc_folder(ansBuffer,ip);
    else if (ip->minor == 1){
        cprintf("proc/");
        if (ip->inum == IDEINFO) {
            cprintf("ideinfo\n");
            //ansLength = ideinfo_File(ansBuffer);
        }
        else if (ip->inum == FILESTAT) {
            cprintf("filestate\n");
            //ansLength = filestat_File(ansBuffer);
        }
        else if (ip->inum == INODEINFO) {
            cprintf("inodeinfo\n");
            //ansLength = inodeinfo_File(ansBuffer);
        }
        else if (ip->inum>=PID_BOTOM && ip->inum<=PID_TOP){
                cprintf("PID\n");
            //ansLength = Pid_folder(ansBuffer);
        }
        else
            panic("not in /proc/");
    }
    else if (ip->minor == 2){
        cprintf("proc/PID/");
        if (ip->inum>=NAME_BOTOM && ip->inum<= NAME_TOP) {
            cprintf("name\n");
            //ansLength = pidname_File(ansBuffer);
        }
        else if (ip->inum>=STATUS_BOTOM && ip->inum <= STATUS_TOP) {
            cprintf("status\n");
            //ansLength = pidstatus_File(ansBuffer);
        }
        else
            panic("not in /proc/pid/");
    }
    if (off<ansLength){
        int tmp_n=ansLength-off;
        if (n<tmp_n)
            tmp_n=n;
        memmove(dst, ansBuffer + off, tmp_n);
        return tmp_n;
    }
    return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n) {
    panic("writing into PROCFS");
    return 0;
}

void
procfsinit(void) {
    devsw[PROCFS].isdir = procfsisdir;
    devsw[PROCFS].iread = procfsiread;
    devsw[PROCFS].write = procfswrite;
    devsw[PROCFS].read = procfsread;
}

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

int sbninodes = 0;

static char *states[] = {
        [UNUSED]    "Unused",
        [EMBRYO]    "Embryo",
        [SLEEPING]  "Sleeping",
        [RUNNABLE]  "Runnable",
        [RUNNING]   "Running",
        [ZOMBIE]    "Zombie"
};

void itoa(char *s, int n) // Convert integer to string
{
    int i = 0, len = 0;
    if (n == 0) {
        s[0] = '0';
        return;
    }
    while (n != 0) {
        s[len] = n % 10 + '0';
        n = n / 10;
        len++;
    }
    for (i = 0; i < len / 2; i++) {
        char tmp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = tmp;
    }
}


void appendDirent(char *buff, char *dirName, int inum, int dirPlace) {
    struct dirent dir;
    dir.inum = inum;
    memmove(&dir.name, dirName, strlen(dirName) + 1);
    memmove(buff + dirPlace * sizeof(dir), (void *) &dir, sizeof(dir));
}

void appendString(char *buff, char *text) {
    int textlen = strlen(text);
    int sz = strlen(buff);
    memmove(buff + sz, text, textlen);
}

void appendNumber(char *buff, int num) {
    char numContainer[10] = {0};
    itoa(numContainer, num);
    appendString(buff, numContainer);
}

int Proc_folder(char *ansBuf) {
    appendDirent(ansBuf, ".", namei("/proc")->inum, 0);
    appendDirent(ansBuf, "..", namei("")->inum, 1);
    appendDirent(ansBuf, "ideinfo", ninodes + 1, 2);
    appendDirent(ansBuf, "inodestat", ninodes + 2, 3);
    appendDirent(ansBuf, "inodeinfo", ninodes + 3, 4);

    // add dirent for every active process
    int dirPlace = 5;
    int pids[NPROC] = {0};
    char numContainer[3] = {0};
    getActiveProc(pids);
    for (int i = 0; i < NPROC; i++)
        if (pids[i] != 0) // if proc in use
        {
            numContainer[0] = 0;
            numContainer[1] = 0;
            numContainer[2] = 0;
            itoa(numContainer, pids[i]);
            appendDirent(ansBuf, numContainer, ninodes + (i + 1) * 100, dirPlace);
            dirPlace++;
        }
    return sizeof(struct dirent) * dirPlace;
}

int Pid_folder(char *ansBuf) {
    short slot = ansBuf[0];
    struct proc *p = getProc(slot);
    char dirPath[9] = {0};
    appendString(dirPath, "/proc/");
    appendNumber(dirPath, p->pid);
    appendDirent(ansBuf, ".", namei(dirPath)->inum, 0);
    appendDirent(ansBuf, "..", namei("/proc")->inum, 1);
    appendDirent(ansBuf, "name", ninodes + 1 + (slot + 1) * 100, 2);
    appendDirent(ansBuf, "status", ninodes + 2 + (slot + 1) * 100, 3);
    return sizeof(struct dirent) * 4;
}

int ideinfo_File(char *ansBuf) {
    //TODO: need to act as if a file
    return 0;
}

int filestat_File(char *ansBuf) {
    //TODO: need to act as if a file
    return 0;
}

int inodeinfo_File(char *ansBuf) {
    //TODO: need to act as if a file
    return 0;
}

int pidname_File(char *ansBuf) {
    int slot = ansBuf[0];
    ansBuf[0] = 0;
    struct proc *p = getProc(slot);
    if (p->pid != 0) {
        appendString(ansBuf, "Process Name: ");
        appendString(ansBuf, p->name);
    }
    return strlen(ansBuf);
}

int pidstatus_File(char *ansBuf) {
    int slot = ansBuf[0];
    ansBuf[0] = 0;
    struct proc *p = getProc(slot);
    if (p->pid != 0) {
        appendString(ansBuf, "Process ");
        appendNumber(ansBuf, p->pid);
        appendString(ansBuf, ":\nState: ");
        appendString(ansBuf, states[p->state]);
        appendString(ansBuf, "\nMemory Usage: ");
        appendNumber(ansBuf, p->sz);
        appendString(ansBuf, " bytes\n");
    }
    return strlen(ansBuf);
}

void initSBninodes(struct inode *ip) {
    if (ninodes != 0)
        return;
    struct superblock sb;
    readsb(ip->dev, &sb);
    sbninodes = sb.ninodes;
}

int
procfsisdir(struct inode *ip) {
    initSBninodes(ip);
    // check if file or is a fake directory
    if (ip->type != T_DEV || ip->major != PROCFS) return 0;
    if (ip->minor == T_DIR) return ip->inum;
    return 0;
}

void
procfsiread(struct inode *dp, struct inode *ip) {
    ip->major = PROCFS;
    ip->type = T_DEV;
    ip->valid = 1;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
    initSBninodes(ip);
    char ansBuffer[1056] = {0}; // longest data is 66 dirents * 16 bytes
    short slot = 0;
    if (ip->inum >= sbninodes + 100) {
        slot = (ip->inum - sbninodes) / 100 - 1;
        ansBuffer[0] = slot;
        short midInum = ip->inum % 100;
        if (midInum >= 10)
            ansBuffer[1] = midInum - 10;
    }
    int ansLength = 0;
    if (ip->inum < sbninodes)                    // proc folder
        ansLength = Proc_folder(ansBuffer);
    if (ip->inum == (sbninodes + 1))                // ideinfo file
        ansLength = ideinfo_File(ansBuffer);
    if (ip->inum == (sbninodes + 2))                // filestat file
        ansLength = filestat_File(ansBuffer);
    if (ip->inum == (sbninodes + 3))                // inodeinfo file
        ansLength = inodeinfo_File(ansBuffer);
    if (ip->inum % 100 == 0)                      // pid folder
        ansLength = Pid_folder(ansBuffer);
    if (ip->inum % 100 == 1)                      // name file
        ansLength = pidname_File(ansBuffer);
    if (ip->inum % 100 == 2)                      // status file
        ansLength = pidstatus_File(ansBuffer);

    memmove(dst, ansBuffer + off, n);
    if (n < ansLength - off) return n;
    return ansLength - off;
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

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


int procFolder(char *ansBuff, struct inode *ip) {
    struct dirent currDirent;
    struct dirent rootDirent;
    struct dirent ideinfoDirent;
    struct dirent filestatDirent;
    struct dirent inodeinfoDirent;
    struct dirent pidDirent;

    currDirent.inum = ip->inum;
    strncpy(currDirent.name, ".", sizeof("."));
    memmove(ansBuff, (char *) &currDirent, sizeof(currDirent));

    rootDirent.inum = ROOTINO;
    strncpy(rootDirent.name, "..", sizeof(".."));
    memmove(ansBuff + sizeof(struct dirent), (char *) &rootDirent, sizeof(rootDirent));

    ideinfoDirent.inum = 300;
    strncpy(ideinfoDirent.name, "ideinfo", sizeof("ideinfo"));
    memmove(ansBuff + sizeof(struct dirent) * 2, (char *) &ideinfoDirent, sizeof(ideinfoDirent));

    filestatDirent.inum = 301;
    strncpy(filestatDirent.name, "filestat", sizeof("filestat"));
    memmove(ansBuff + sizeof(struct dirent) * 3, (char *) &filestatDirent, sizeof(filestatDirent));

    inodeinfoDirent.inum = 302;
    strncpy(inodeinfoDirent.name, "inodeinfo", sizeof("inodeinfo"));
    memmove(ansBuff + sizeof(struct dirent) * 4, (char *) &inodeinfoDirent, sizeof(inodeinfoDirent));

    int dirToAdd = 5;
    int pids[NPROC] = {0};
    getActiveProc(pids);
    for (int i = 0; i < NPROC; i++)
        if (pids[i] != 0) {
            pidDirent.inum = 303 + i;
            itoa(pidDirent.name, pids[i], 10);
            memmove(ansBuff + sizeof(struct dirent) * dirToAdd, (char *) &pidDirent, sizeof(pidDirent));
            dirToAdd++;
        }
    return dirToAdd * sizeof(struct dirent);
}

int procPidFolder(char *ansBuffer, struct inode *ip) {
    struct dirent currDirent;
    struct dirent rootDirent;
    struct dirent nameDirent;
    struct dirent statusDirent;

    currDirent.inum = ip->inum;
    strncpy(currDirent.name, ".", sizeof("."));
    memmove(ansBuffer, (char *) &currDirent, sizeof(currDirent));

    rootDirent.inum = 400;
    strncpy(rootDirent.name, "..", sizeof(".."));
    memmove(ansBuffer + sizeof(struct dirent), (char *) &rootDirent, sizeof(rootDirent));

    nameDirent.inum = ip->inum - 303 + 401;
    strncpy(nameDirent.name, "name", strlen("name") + 1);
    memmove(ansBuffer + sizeof(struct dirent) * 2, (char *) &nameDirent, sizeof(nameDirent));

    statusDirent.inum = ip->inum - 303 + 501;
    strncpy(statusDirent.name, "status", strlen("status") + 1);
    memmove(ansBuffer + sizeof(struct dirent) * 3, (char *) &statusDirent, sizeof(statusDirent));

    return 4 * sizeof(struct dirent);
}

int nameFolder(char *ansBuff, struct inode *ip) {
    char *name = getPtable()[ip->inum - 401].name;

    strncpy(ansBuff, "Process name: ", strlen("Process name: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), name, strlen(name) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);
    return strlen(ansBuff);
}

int statusFolder(char *ansBuff, struct inode *ip) {
    static char *states[] = {
            [UNUSED]    "Unused",
            [EMBRYO]    "Embryo",
            [SLEEPING]  "Sleeping",
            [RUNNABLE]  "Runnable",
            [RUNNING]   "Running",
            [ZOMBIE]    "Zombie"
    };
    uint memoryUsage = getPtable()[ip->inum - 501].sz;
    int index = (int) getPtable()[ip->inum - 501].state;
    char memoryUsageStr[10] = {0};
    itoa(memoryUsageStr, memoryUsage, 10);
    strncpy(ansBuff, "Process run state: ", strlen("Process run state: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), states[index], strlen(states[index]) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);
    strncpy(ansBuff + strlen(ansBuff), "Process memory usage: ", strlen("Process memory usage: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), memoryUsageStr, strlen(memoryUsageStr) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);
    return strlen(ansBuff);
}

int ideinfoFile(char *ansBuff) {
    int nwo = numOfWritingOps();
    int nrwo = numOfReadWaitingOps();
    int nwwo = numWriteWaitingOps();
    char *nl = "\n";
    char *tmp = "Waiting operations: ";

    char wat[10];
    itoa(wat, nwo, 10);
    strncpy(ansBuff, tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), wat, strlen(wat) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);
    char r[10];
    itoa(r, nrwo, 10);
    tmp = "Read waiting operations: ";
    strncpy(ansBuff + strlen(ansBuff), tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), r, strlen(r) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);
    char w[10];
    itoa(w, nwwo, 10);
    tmp = "Write waiting operations: ";
    strncpy(ansBuff + strlen(ansBuff), tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), w, strlen(w) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);
    return strlen(ansBuff);
}

int filestatFile(char *ansBuff) {
    int nffds = numOfFreeFDS();
    int nuifds = numOfUniqueFDS();
    int nwfds = numOfWriteableFDS();
    int nrfds = numOFReadableFDS();
    int rpfds = numOfRafs();
    char *nl = "\n";

    char *tmp = "Free fds: ";
    char freeFds[10];
    itoa(freeFds, nffds, 10);
    strncpy(ansBuff, tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), freeFds, strlen(freeFds) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);

    tmp = "Unique inode fds: ";
    char unique[10];
    itoa(unique, nuifds, 10);
    strncpy(ansBuff + strlen(ansBuff), tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), unique, strlen(unique) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);

    tmp = "Writeable fds: ";
    char writeable[10];
    itoa(writeable, nwfds, 10);
    strncpy(ansBuff + strlen(ansBuff), tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), writeable, strlen(writeable) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);

    tmp = "Readable fds: ";
    char readable[10];
    itoa(readable, nrfds, 10);
    strncpy(ansBuff + strlen(ansBuff), tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), readable, strlen(readable) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);

    tmp = "Refs per fds: ";
    char refsPerFds[10];
    itoa(refsPerFds, rpfds, 10);
    strncpy(ansBuff + strlen(ansBuff), tmp, strlen(tmp) + 1);
    strncpy(ansBuff + strlen(ansBuff), refsPerFds, strlen(refsPerFds) + 1);
    strncpy(ansBuff + strlen(ansBuff), nl, strlen(nl) + 1);

    return strlen(ansBuff);
}

int inodeinfoFile(char *ansBuff, struct inode *ip) {
    struct dirent currDirent;
    struct dirent rootDirent;
    struct dirent inodeinfoDirent;
    struct inode *n;

    currDirent.inum = ip->inum;
    strncpy(currDirent.name, ".", sizeof("."));
    memmove(ansBuff, (char *) &currDirent, sizeof(currDirent));

    rootDirent.inum = 600;
    strncpy(rootDirent.name, "..", sizeof(".."));
    memmove(ansBuff + sizeof(struct dirent), (char *) &rootDirent, sizeof(rootDirent));

    int i = 0;
    int dirToAdd = 2;
    for (n = get_icache(); n < &get_icache()[NINODE]; n++) {
        if (n->ref != 0) { //   in use
            inodeinfoDirent.inum = 601 + i;
            itoa(inodeinfoDirent.name, i, 10);
            memmove(ansBuff + sizeof(struct dirent) * dirToAdd, (char *) &inodeinfoDirent, sizeof(inodeinfoDirent));
            dirToAdd++;
        }
        i++;
    }
    return dirToAdd * sizeof(struct dirent);
}

int inodeinfoFolder(char *ansBuff, struct inode *ip) {
    struct inode *n = &get_icache()[ip->inum - 601]; // index in the icache of the curr inode

    char dev[10];
    itoa(dev, n->dev, 10);
    strncpy(ansBuff, "Device: ", strlen("Device: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), dev, strlen(dev) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    char inum[10];
    itoa(inum, n->inum, 10);
    strncpy(ansBuff + strlen(ansBuff), "Inode number: ", strlen("Inode number: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), inum, strlen(inum) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    char valid[10];
    itoa(valid, n->valid, 10);
    strncpy(ansBuff + strlen(ansBuff), "is valid: ", strlen("is valid: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), valid, strlen(valid) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    char type[10];
    if (n->type == 1)
        strncpy(type, "DIR", strlen("DIR") + 1);
    else if (n->type == 2)
        strncpy(type, "FILE", strlen("FILE") + 1);
    else if (n->type == 3)
        strncpy(type, "DEV", strlen("DEV") + 1);
    strncpy(ansBuff + strlen(ansBuff), "type: ", strlen("type: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), type, strlen(type) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    char major[10];
    char minor[10];
    itoa(major, n->major, 10);
    itoa(minor, n->minor, 10);
    strncpy(ansBuff + strlen(ansBuff), "major minor: ", strlen("major minor: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), "(", strlen("(") + 1);
    strncpy(ansBuff + strlen(ansBuff), major, strlen(major) + 1);
    strncpy(ansBuff + strlen(ansBuff), ", ", strlen(", ") + 1);
    strncpy(ansBuff + strlen(ansBuff), minor, strlen(minor) + 1);
    strncpy(ansBuff + strlen(ansBuff), ")", strlen(")") + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    char hLink[10];
    itoa(hLink, n->nlink, 10);
    strncpy(ansBuff + strlen(ansBuff), "hard links: ", strlen("hard links: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), hLink, strlen(hLink) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    char blocks[10];
    itoa(blocks, (n->size) / BSIZE + 1, 10);
    strncpy(ansBuff + strlen(ansBuff), "blocks used: ", strlen("blocks used: ") + 1);
    strncpy(ansBuff + strlen(ansBuff), blocks, strlen(blocks) + 1);
    strncpy(ansBuff + strlen(ansBuff), "\n", strlen("\n") + 1);

    return strlen(ansBuff);
}

int
procfsisdir(struct inode *ip) {
    if (ip->major == PROCFS && ip->type == T_DEV) {
        if ((ip->minor == 0) || (ip->minor == 1 && ip->inum >= 302 && ip->inum < 400) ||
            (ip->minor == 2 && ip->inum >= 601)) // it is directory
            return 1;
    }
    return 0;
}

void
procfsiread(struct inode *dp, struct inode *ip) {
    ip->ref = 1;
    ip->type = T_DEV;
    ip->major = PROCFS;

    if (ip->inum < 300)
        ip->minor = 0;
    else if (ip->inum >= 300 && ip->inum < 400)
        ip->minor = 1;
    else if (ip->inum >= 400)
        ip->minor = 2;

    ip->nlink = 1; //not sure
    ip->valid = 1;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
    int size = 0;
    char ansBuff[sizeof(struct dirent) * NPROC + sizeof(struct dirent) * NINODE + 1000];

    if (ip->minor == 0)     // proc folder
        size = procFolder(ansBuff, ip);
    else if (ip->minor == 1) {          // proc/..
        if (ip->inum == 300)                        // ideinfo
            size = ideinfoFile(ansBuff);
        else if (ip->inum == 301)                   // filestat
            size = filestatFile(ansBuff);
        else if (ip->inum == 302)                   // inodeinfo
            size = inodeinfoFile(ansBuff, ip);
        else if (ip->inum >= 303 && ip->inum < 400) //proc/PID
            size = procPidFolder(ansBuff, ip);
    } else if (ip->minor == 2) {                        // proc/PID/..  proc/inodeinfo/..
        if (ip->inum >= 401 && ip->inum < 501)              // proc/PID/name
            size = nameFolder(ansBuff, ip);
        else if (ip->inum >= 501 && ip->inum < 600)         // proc/PID/status
            size = statusFolder(ansBuff, ip);
        else if (ip->inum >= 601 && ip->inum < 700)         // proc/inodeinfo/PID
            size = inodeinfoFolder(ansBuff, ip);
    }
    if (off < size) {
        if (n > size - off)
            n = size - off;
        memmove(dst, ansBuff + off, n);
        return n;
    }
    return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n) {
    panic("only read system");
    return 0;
}

void
procfsinit(void) {
    devsw[PROCFS].isdir = procfsisdir;
    devsw[PROCFS].iread = procfsiread;
    devsw[PROCFS].write = procfswrite;
    devsw[PROCFS].read = procfsread;
}

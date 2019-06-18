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

int 
procfsisdir(struct inode *ip) {
    if(ip->major == PROCFS && ip->type == T_DEV){
        if((ip->minor == 0) || (ip->minor == 1 && ip->inum >= 302 && ip->inum <400) || (ip->minor == 2 && ip->inum >= 601)) //it's a directory
            return 1;
    }
    return 0; //not a directory
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
    ip->ref = 1;
    ip->type = T_DEV;
    ip->major = PROCFS;
    ip->nlink = 1;
    ip->valid = 1;
    if(ip->inum < 300) //the first layer (proc)
        ip->minor = 0;
    else if(ip->inum >= 300 && ip->inum < 400) //the inum for the second layer (proc/...)
        ip->minor = 1;
    else if(ip->inum >= 400) //the inum for the third layer (proc/PID/...)
        ip->minor = 2;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
    struct dirent currDirent;
    struct dirent rootDirent;
    struct proc* p;
    int size = 0;
    char temp_buf[sizeof(struct dirent) * NPROC + sizeof(struct dirent) * NINODE + 1000];

    if(ip->minor == 0){ //proc
        struct dirent ideinfoDirent;
        struct dirent filestatDirent;
        struct dirent inodeinfoDirent;
        struct dirent pidDirent;

        currDirent.inum = ip->inum;
        strncpy(currDirent.name, ".", sizeof("."));
        memmove(temp_buf, (char*)&currDirent, sizeof(currDirent));

        rootDirent.inum = ROOTINO;
        strncpy(rootDirent.name, "..", sizeof(".."));
        memmove(temp_buf + sizeof(struct dirent), (char*)&rootDirent, sizeof(rootDirent));

        ideinfoDirent.inum = 300;
        strncpy(ideinfoDirent.name, "ideinfo", sizeof("ideinfo"));
        memmove(temp_buf + sizeof(struct dirent) * 2, (char*)&ideinfoDirent, sizeof(ideinfoDirent));

        filestatDirent.inum = 301;
        strncpy(filestatDirent.name, "filestat", sizeof("filestat"));
        memmove(temp_buf + sizeof(struct dirent) * 3, (char*)&filestatDirent, sizeof(filestatDirent));

        inodeinfoDirent.inum = 302;
        strncpy(inodeinfoDirent.name, "inodeinfo", sizeof("inodeinfo"));
        memmove(temp_buf + sizeof(struct dirent) * 4, (char*)&inodeinfoDirent, sizeof(inodeinfoDirent));

        int i = 0;
        int counter = 5;
        for(p = get_ptable(); p < &get_ptable()[NPROC]; p++){
            if(p->state != UNUSED && p->state != ZOMBIE){
                pidDirent.inum = 303 + i;
                itoa(pidDirent.name , p->pid,10);
                memmove(temp_buf + sizeof(struct dirent) * counter, (char*)&pidDirent, sizeof(pidDirent));
                counter++;
            }
            i++;
        }
        size = counter * sizeof(struct dirent);
    }
    else if(ip->minor == 1){ //proc/...
        if(ip->inum == 300){ //ideinfo
            //Waiting operations: <Number of waiting operations starting from idequeue>
            char waiting[10];
            itoa(waiting,getWaitingOperations(),10);
            strncpy(temp_buf, "Waiting operations: ", strlen("Waiting operations: ")+1);
            size += strlen("Waiting operations: ")+1;
            strncpy(temp_buf + strlen(temp_buf), waiting, strlen(waiting)+1);
            size += strlen(waiting)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Read waiting operations: <Number of read operations>
            char read[10];
            itoa(read,getReadWaitingOperations(),10);
            strncpy(temp_buf + strlen(temp_buf), "Read waiting operations: ", strlen("Read waiting operations: ")+1);
            size += strlen("Read waiting operations: ")+1;
            strncpy(temp_buf + strlen(temp_buf), read, strlen(read)+1);
            size += strlen(read)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Write waiting operations: <Number of write operations>
            char write[10];
            itoa(write,getWriteWaitingOperations(),10);
            strncpy(temp_buf + strlen(temp_buf), "Write waiting operations: ", strlen("Write waiting operations: ")+1);
            size += strlen("Write waiting operations: ")+1;
            strncpy(temp_buf + strlen(temp_buf), write, strlen(write)+1);
            size += strlen(write)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Working blocks: <List (#device,#block) that are currently in the queue separated by the ‘;’symbol>
            strncpy(temp_buf + strlen(temp_buf), "Working blocks: ", strlen("Working blocks: ")+1);
            size += strlen("Working blocks: ")+1;
            strncpy(temp_buf + strlen(temp_buf), getWorkingBlocks(), strlen(getWorkingBlocks())+1);
            size += strlen(getWorkingBlocks())+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

        }
        else if(ip->inum == 301){ //filestat
            //Free fds: <free fd number (ref = 0)>
            char freeFds[10];
            itoa(freeFds,getNumberOfFreeFds(),10);
            strncpy(temp_buf, "Free fds: ", strlen("Free fds: ")+1);
            size += strlen("Free fds: ")+1;
            strncpy(temp_buf + strlen(temp_buf), freeFds, strlen(freeFds)+1);
            size += strlen(freeFds)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Unique inode fds: <Number of different inodes open by all the fds>
            char unique[10];
            itoa(unique,getUniqueInodeFds(),10);
            strncpy(temp_buf + strlen(temp_buf), "Unique inode fds: ", strlen("Unique inode fds: ")+1);
            size += strlen("Unique inode fds: ")+1;
            strncpy(temp_buf + strlen(temp_buf), unique, strlen(unique)+1);
            size += strlen(unique)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Writeable fds: <Writable fd number>
            char writeable[10];
            itoa(writeable,getWriteableFdNumber(),10);
            strncpy(temp_buf + strlen(temp_buf), "Writeable fds: ", strlen("Writeable fds: ")+1);
            size += strlen("Writeable fds: ")+1;
            strncpy(temp_buf + strlen(temp_buf), writeable, strlen(writeable)+1);
            size += strlen(writeable)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Readable fds: <Readable fd number>
            char readable[10];
            itoa(readable,getReadableFdNumber(),10);
            strncpy(temp_buf + strlen(temp_buf), "Readable fds: ", strlen("Readable fds: ")+1);
            size += strlen("Readable fds: ")+1;
            strncpy(temp_buf + strlen(temp_buf), readable, strlen(readable)+1);
            size += strlen(readable)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Refs per fds: <ratio of total number of refs / number of used fds>
            char refsPerFds[10];
            itoa(refsPerFds,getRefsPerFds(),10);
            strncpy(temp_buf + strlen(temp_buf), "Refs per fds: ", strlen("Refs per fds: ")+1);
            size += strlen("Refs per fds: ")+1;
            strncpy(temp_buf + strlen(temp_buf), refsPerFds, strlen(refsPerFds)+1);
            size += strlen(refsPerFds)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

        }
        else if(ip->inum == 302){ //inodeinfo
            struct dirent inodeinfoDirent;
            struct inode* n;

            currDirent.inum = ip->inum;
            strncpy(currDirent.name, ".", sizeof("."));
            memmove(temp_buf, (char*)&currDirent, sizeof(currDirent));

            rootDirent.inum = 600;
            strncpy(rootDirent.name, "..", sizeof(".."));
            memmove(temp_buf + sizeof(struct dirent), (char*)&rootDirent, sizeof(rootDirent));

            int i = 0;
            int counter = 2;
            for(n = get_icache(); n < &get_icache()[NINODE]; n++){
                if(n->ref != 0){ //in use
                    inodeinfoDirent.inum = 601 + i;
                    itoa(inodeinfoDirent.name,i,10);
                    memmove(temp_buf + sizeof(struct dirent) * counter, (char*)&inodeinfoDirent, sizeof(inodeinfoDirent));
                    counter++;
                }
                i++;
            }
            size = counter * sizeof(struct dirent);
        }
        else if(ip->inum >= 303 && ip->inum <400){ //proc/PID
            struct dirent nameDirent;
            struct dirent statusDirent;

            currDirent.inum = ip->inum;
            strncpy(currDirent.name, ".", sizeof("."));
            memmove(temp_buf, (char*)&currDirent, sizeof(currDirent));

            rootDirent.inum = 400;
            strncpy(rootDirent.name, "..", sizeof(".."));
            memmove(temp_buf + sizeof(struct dirent), (char*)&rootDirent, sizeof(rootDirent));

            nameDirent.inum = ip->inum - 303 + 401;
            strncpy(nameDirent.name, "name", 6);
            memmove(temp_buf + sizeof(struct dirent) * 2, (char*)&nameDirent, sizeof(nameDirent));

            statusDirent.inum = ip->inum - 303 + 501;
            strncpy(statusDirent.name, "status", 8);
            memmove(temp_buf + sizeof(struct dirent) * 3, (char*)&statusDirent, sizeof(statusDirent));

            size = 4 * sizeof(struct dirent);
        }
    }
    else if(ip->minor == 2){ //proc/PID/... or proc/inodeinfo/...
        if(ip->inum >= 401 && ip->inum < 501){ //name
            struct proc *currproc = &get_ptable()[ip->inum - 401]; // index in the ptable of the current proc

            // get the name of the process
            char name_proc[16];
            strncpy(name_proc, currproc->name, strlen(currproc->name)+1);
            strncpy(temp_buf, "The process name: ", strlen("The process name: ")+1);
            size += strlen("The process name: ")+1;
            strncpy(temp_buf + strlen(temp_buf), name_proc, strlen(name_proc)+1);
            size += strlen(name_proc)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

        }
        else if(ip->inum >= 501 && ip->inum < 600){ //status
            struct proc *currproc = &get_ptable()[ip->inum - 501]; // index in the ptable of the current proc

            // get the state of the process
            char state_proc[10];
            if (currproc->state == UNUSED)
                strncpy(state_proc, "UNUSED", strlen("UNUSED")+1);
            else if (currproc->state == EMBRYO)
                strncpy(state_proc, "EMBRYO", strlen("EMBRYO")+1);
            else if (currproc->state == SLEEPING)
                strncpy(state_proc, "SLEEPING", strlen("SLEEPING")+1);
            else if (currproc->state == RUNNABLE)
                strncpy(state_proc, "RUNNABLE", strlen("RUNNABLE")+1);
            else if (currproc->state == RUNNING)
                strncpy(state_proc, "RUNNING", strlen("RUNNING")+1);
            else if (currproc->state == ZOMBIE)
                strncpy(state_proc, "ZOMBIE", strlen("ZOMBIE")+1);

            strncpy(temp_buf, "The process state: ", strlen("The process state: ")+1);
            size += strlen("The process state: ")+1;
            strncpy(temp_buf + strlen(temp_buf), state_proc, strlen(state_proc)+1);
            size += strlen(state_proc)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //get the size of the process
            char size_proc[10];
            itoa(size_proc,currproc->sz,10);
            strncpy(temp_buf + strlen(temp_buf), "The memory usage of the process: ", strlen("The memory usage of the process: ")+1);
            size += strlen("The memory usage of the process: ")+1;
            strncpy(temp_buf + strlen(temp_buf), size_proc, strlen(size_proc)+1);
            size += strlen(size_proc)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;
        }
        else if(ip->inum >= 601 && ip->inum < 700){ //proc/inodeinfo/...
            struct inode *n = &get_icache()[ip->inum - 601]; // index in the icache of the curr inode

            //Device: <device the inode belongs to>
            char dev[10];
            itoa(dev,n->dev,10);
            strncpy(temp_buf, "Device: ", strlen("Device: ")+1);
            size += strlen("Device: ")+1;
            strncpy(temp_buf + strlen(temp_buf), dev, strlen(dev)+1);
            size += strlen(dev)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //Inode number: <inode number in the device>
            char inum[10];
            itoa(inum,n->inum,10);
            strncpy(temp_buf + strlen(temp_buf), "Inode number: ", strlen("Inode number: ")+1);
            size += strlen("Inode number: ")+1;
            strncpy(temp_buf + strlen(temp_buf), inum, strlen(inum)+1);
            size += strlen(inum)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //is valid: <0 for no, 1 for yes>
            char valid[10];
            itoa(valid,n->valid,10);
            strncpy(temp_buf + strlen(temp_buf), "is valid: ", strlen("is valid: ")+1);
            size += strlen("is valid: ")+1;
            strncpy(temp_buf + strlen(temp_buf), valid, strlen(valid)+1);
            size += strlen(valid)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //type: <DIR, FILE or DEV>
            char type[10];
            if(n->type == 1){
                strncpy(type, "DIR", strlen("DIR")+1);
            }
            else if(n->type == 2){
                strncpy(type, "FILE", strlen("FILE")+1);
            }
            else if(n->type == 3) {
                strncpy(type, "DEV", strlen("DEV") + 1);
            }
            strncpy(temp_buf + strlen(temp_buf), "type: ", strlen("type: ")+1);
            size += strlen("type: ")+1;
            strncpy(temp_buf + strlen(temp_buf), type, strlen(type)+1);
            size += strlen(type)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //major minor: <(major number, minor number)>
            char major[10];
            char minor[10];
            itoa(major,n->major,10);
            itoa(minor,n->minor,10);
            strncpy(temp_buf + strlen(temp_buf), "major minor: ", strlen("major minor: ")+1);
            size += strlen("major minor: ")+1;
            strncpy(temp_buf + strlen(temp_buf), "(", strlen("(")+1);
            size += strlen("(")+1;
            strncpy(temp_buf + strlen(temp_buf), major, strlen(major)+1);
            size += strlen(major)+1;
            strncpy(temp_buf + strlen(temp_buf), ", ", strlen(", ")+1);
            size += strlen(", ")+1;
            strncpy(temp_buf + strlen(temp_buf), minor, strlen(minor)+1);
            size += strlen(minor)+1;
            strncpy(temp_buf + strlen(temp_buf), ")", strlen(")")+1);
            size += strlen(")")+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //hard links: <number of hardlinks>
            char hLink[10];
            itoa(hLink,n->nlink,10);
            strncpy(temp_buf + strlen(temp_buf), "hard links: ", strlen("hard links: ")+1);
            size += strlen("hard links: ")+1;
            strncpy(temp_buf + strlen(temp_buf), hLink, strlen(hLink)+1);
            size += strlen(hLink)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

            //blocks used: <number of blocks used in the file, 0 for DEV files>
            char block[10];
            itoa(block,(n->size)/BSIZE + 1,10);
            strncpy(temp_buf + strlen(temp_buf), "blocks used: ", strlen("blocks used: ")+1);
            size += strlen("blocks used: ")+1;
            strncpy(temp_buf + strlen(temp_buf), block, strlen(block)+1);
            size += strlen(block)+1;
            strncpy(temp_buf + strlen(temp_buf), "\n", strlen("\n")+1);
            size += strlen("\n")+1;

        }
    }

    if (off < size) {
        int newN = size - off;
        if(n < newN)
            newN = n;
        memmove(dst, temp_buf + off, newN);
        return newN;
    }
    return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
  return 0; //error- because the system shouldn't write, only read
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}

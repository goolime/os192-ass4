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

int sbninodes=0;

int Proc_folder(char *ansBuf){
  //TODO: need to act as if a folder
  return 0;
}

int Pid_folder(char *ansBuf){
  //TODO: need to act as if a folder
  return 0;
}

int ideinfo_File(char *ansBuf){
  //TODO: need to act as if a file
  return 0;
}

int filestat_File(char *ansBuf){
  //TODO: need to act as if a file
  return 0;
}

int inodeinfo_File(char *ansBuf){
  //TODO: need to act as if a file
  return 0;
}

int pidname_File(char *ansBuf){
  //TODO: need to act as if a file
  return 0;
}

int pidstatus_File(char *ansBuf){
  //TODO: need to act as if a file
  return 0;
}

void initSBninodes(struct inode *ip){
  struct superblock sb;
  readsb(ip->dev, &sb);
  sbninodes = sb.ninodes;
}

int 
procfsisdir(struct inode *ip) {
  //initSBninodes(ip);
  // check if file is a fake directory
  if (ip->type != T_DEV || ip->major != PROCFS) return 0;
  if (ip->minor == T_DIR) return ip->inum;
  return 0;
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
  ip->major = PROCFS;
  ip->type = T_DEV;
  ip->valid=1;
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
  initSBninodes(ip);
  char ansBuffer[1056] = {0}; //longest data is 66 dirents * 16 bytes
  short slot = 0;
  if (ip->inum >= sbninodes+100){
    slot = (ip->inum-sbninodes)/100 - 1;
    ansBuffer[0] = slot;
    short midInum = ip->inum % 100;
    if (midInum >= 10)
      ansBuffer[1] = midInum-10;
  }
  int ansLength=0;
  if (ip->inum < sbninodes) 					    // proc folder
    ansLength = Proc_folder(ansBuffer);
  if (ip->inum == (sbninodes+1))			        // ideinfo file
    ansLength = ideinfo_File(ansBuffer);
  if (ip->inum == (sbninodes+2))		    	    // filestat file
    ansLength = filestat_File(ansBuffer);
  if (ip->inum == (sbninodes+3))			        // inodeinfo file
    ansLength = inodeinfo_File(ansBuffer);
  if (ip->inum % 100 == 0)
    ansLength = Pid_folder(ansBuffer);	    // pid folder
  if (ip->inum % 100 == 1)
    ansLength = pidname_File(ansBuffer);             // name file
  if (ip->inum % 100 == 2)
    ansLength = pidstatus_File(ansBuffer);			// status file

  memmove(dst, ansBuffer+off, n);
  if (n< ansLength-off) return n;
  return ansLength-off;
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
  panic("writing into PROCFS");
  return 0;
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}

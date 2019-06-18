#include "types.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

//return 0 on success, and -1 on error
int lsnd(){
    struct dirent dirent; //the dirent that was read
    int fd; //fd to the /proc/inodeinfo content
    int n;
    char buff[600];
    int inodefd = -1; //fd to the /proc/inodeinfo/name content
    char content[600];
    int i;
    char row[300]; //spesific row content
    int rowLen = 0;
    int j;
    char contentToPrint[200]; //the content after ':' of every row
    int contentToPrintLen = 0;
    int k;


    //Read the /proc/inodeinfo
    fd = open("/proc/inodeinfo", O_RDONLY);
    if(fd < 0) //couldn't read
        return -1;

    //Go over all the indoes under /proc/inodeinfo
    n = read(fd, &dirent, sizeof(dirent));
    if(n <= 0){
        printf(1, "The dirent is empty");
        return -1;
    }
    while(n > 0){
        if(dirent.inum >= 601){ //inode under /proc/inodeinfo
            //create proc/inodeinfo/name
            memmove(buff,"/proc/inodeinfo/", strlen("/proc/inodeinfo/"));
            memmove(buff + strlen("/proc/inodeinfo/"), dirent.name, strlen(dirent.name)+1);
            
            //Open the proc/inodeinfo/name
            inodefd = open(buff, O_RDONLY);
            if(inodefd < 0) {
                break;
            }

            //Read the content of proc/inodeinfo/name
            n = read(inodefd, content, 600);
            if(n <= 0) {
                return -1;
            }

            //Parse the content
            for(i=0; i<strlen(content); i++){
                if(content[i] != '\n'){ //not the end of spesific row
                    //memset(row + rowLen, content[i], 1);
                    row[rowLen] = content[i];
                    rowLen++;
                }
                else{ //the end of a row- so parse the row that just was ended
                    for(j=0; j<rowLen; j++){ //go over the row  
                        if(row[j] == ':'){
                            for(k=j+1; k<rowLen; k++){ //save whats after ':' in contentToPrint
                                //memset(contentToPrint + contentToPrintLen, row[k], 1);
                                contentToPrint[contentToPrintLen] = row[k];
                                contentToPrintLen++;
                            }
                            contentToPrint[contentToPrintLen] ='\0';
                            printf(1, "%s", contentToPrint);

                            //initialize contentToPrint (prepare it to the next row)
                            for (k = 0; k < contentToPrintLen; k++) {
                                contentToPrint[k] = 0;
                            }
                            contentToPrintLen = 0;
                        }
                    }
                    //initialize contentToPrint (prepare it to the next row)
                    for (j = 0; j < rowLen; j++) {
                        row[j] = 0;
                    }
                    rowLen = 0;
                }
            }
            printf(1, "\n");
        }
        n = read(fd, &dirent, sizeof(dirent)); //read the next inode
    }
    close(fd);
    close(inodefd);
    return 0;
}

int
main(int argc, char *argv[])
{
    int ans = lsnd();
    if(ans == -1)
        printf(1, "Test failed\n");
    else
        printf(1, "Test passed\n");
   exit();
}
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define COUNT 100


int main(int argc, char *argv[]) {

    char buf[COUNT];
    if(argc !=3)
    {
        perror("can't complete cp operation");
        exit(-1);
    }
    int fd_src = open(argv[1],O_RDONLY);
    if(fd_src < 0)
	{
		printf("couldn't open the file\n");
		exit(-2);
	}
    int fd_des = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC , 0644);
    int num_read;
    while (num_read = read(fd_src,buf,COUNT))
    {
        if(write(fd_des,buf, num_read)< 0 )
        {
            printf("Write failed\n");
			exit(-3);
        }
    }
    close(fd_src);
    remove(argv[1]); // remove source file
    close(fd_des);
    
    return 0;
}
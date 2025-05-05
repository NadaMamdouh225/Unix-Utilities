#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define COUNT 100

int main(int argc, char** argv)
{
	char buf[COUNT];
	if(argc != 2)
	{
		printf("Usage %s file-name \n",argv[0]);
		exit(-1);
	}
	int fd = open(argv[1],O_RDONLY);
	if(fd < 0)
	{
		printf("couldn't open the file\n");
		exit(-2);
	}
	int num_read ;
	while ((num_read = read(fd, buf, COUNT))>0){
	
		if(write(1, buf, num_read)<0)
		{
			printf("Write failed\n");
			exit(-3);
		}
	}
	close(fd);
	return 0;
}


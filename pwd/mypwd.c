#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>



int main(int argc, char **argv)
{
    char* buf = getcwd(NULL,0);
    if( buf == NULL)
    {
        perror("Couldn't get current working directory");
        exit(-1);
    }
    printf("%s\n",buf);
    free(buf);
    return 0;
}
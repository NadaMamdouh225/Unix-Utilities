#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    for(int i=1;i<argc;i++)
    {
        if(printf("%s",argv[i])<0)
        {   
            perror("couldn't echo");
            exit(-1);
        }
        if(i<argc-1)
        {
            if(printf(" ")<0){
            perror("faild printing spaces");
            exit(-1);
            }

        }
    }
    
    if(printf("\n")<0)
    {
        perror("failed inserting new line");
        exit(-2);

    }
    return 0;

}


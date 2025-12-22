#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char buffer[10500];
char *cmd;
char *arg;

int getCmd(void)
{
    // EOF
    if(fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;
    
    cmd = strtok(buffer, " \n");
    arg = strtok(NULL, "\n");
  
    return 1;   
}

int femtoshell_main(int argc, char *argv[]) {
    int status = 0;
    while(1)
    {
        const char *shellmsg = "Femto shell prompt > ";
        write(STDOUT_FILENO, shellmsg, strlen(shellmsg));
        

        if(getCmd() == 0)
            exit(status);
        

         // handle empty line 
        if (cmd == NULL)
            continue;

        if(strcmp("echo", cmd)==0) 
        {
            if(arg != NULL)
                write(1, arg, strlen(arg));
            write(1, "\n", 1);
        }   
        else if(strcmp("exit", cmd)==0)
        {
                const char * msg = "Good Bye\n";
                write(1, msg, strlen(msg));
                exit(status);
        }
        else{
            write(STDOUT_FILENO, "Invalid command\n", 16);
            status = 1;
            
        }
        
    }
    return 0;
}
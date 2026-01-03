#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


char *cmd;
char *arg;
int status = 0;

char* handle_spaces(char *inBuf)
{
    int inIndex = 0, outIndex = 0;
    int in_space = 0;

    if(inBuf == NULL)
        return NULL;

    char* outBuf = (char*)malloc(strlen(inBuf)+1);

    if(outBuf == NULL) {
        perror("malloc failed");
        return NULL;
    }

    while( inBuf[inIndex] != '\0')
    {
        if(inBuf[inIndex] == ' ')
        {
            if(!in_space)
            {
                outBuf[outIndex++] = ' ';
                in_space = 1;
            }
        }else{
            outBuf[outIndex++] =  inBuf[inIndex];
            in_space = 0;
        }
        inIndex++;   
        
    }

    outBuf[outIndex] = '\0';
    return outBuf;
}

int getCmd(void)
{
    static char *buffer = NULL;
    static size_t buffer_size = 0;
    static char *outBuffer = NULL;  // ✓ Make static to persist

    // Free previous allocation
    if(outBuffer != NULL)
    {
        free(outBuffer);
        outBuffer = NULL;
    }

    // Read line with automatic allocation
    ssize_t len = getline(&buffer, &buffer_size, stdin);
    if(len == -1)
    {
        if(buffer != NULL)
        {
            free(buffer);
            buffer = NULL;
        }
        return 0;
    }

    outBuffer = handle_spaces(buffer);
    if(outBuffer == NULL)
        return 0;

    cmd = strtok(outBuffer, " \n");
    arg = strtok(NULL, "\n");

    return 1;
}

void pwd_cmd(void)
{
    char *buf = getcwd(NULL, 0);
    if(buf == NULL)
    {
        const char * msg = "Couldn't get current working directory\n";
        write(STDERR_FILENO, msg, strlen(msg));
        status = 1;
        return;
    }

    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, "\n", 1);
    free(buf);
    status = 0;
}
void cd_cmd(void)
{
    if(arg == NULL)
    {
        const char *home = getenv("HOME");
        int error_state = chdir(home);
        if (home == NULL || error_state!= 0)
        {
            const char *msg = "cd: HOME not set\n";
            write(STDERR_FILENO, msg, strlen(msg));
            status = error_state;
        }
        }else{
            int error_state = chdir(arg);
            if( error_state != 0){
                const char * msg = "cd: /invalid_directory: No such file or directory\n";
                write(STDERR_FILENO, msg, strlen(msg));
                status = error_state;
            }
    }
}

void execute_custom_cmd(void)
{
    pid_t pid = fork();
            
            if(pid > 0)    // if parent then wait
            {
                int s;
                wait(&s);
                if (WIFEXITED(s))
                    status = WEXITSTATUS(s);
                else
                    status = 1;
            }else if(pid == 0)      // if child then execvp
            {
                // count arguments
                int arg_count = 0;
                if(arg != NULL)
                {
                    char *temp = strdup(arg);  // Duplicate to count without modifying
                    if(temp == NULL)
                    {
                        perror("strdup failed");
                        exit(1);
                    }
                    char *token = strtok(temp, " ");
                    while(token != NULL)
                    {
                        arg_count++;
                        token = strtok(NULL, " ");
                    }
                    free(temp);
                }


                char **newargv = (char**)malloc((arg_count + 1)*sizeof(char*));
                if(newargv == NULL)
                {
                    perror("malloc failed");
                    exit(1);
                }

                newargv[0] = cmd;

                int i = 1;
                char* token = strtok(arg," ");
                while(token != NULL)
                {
                    newargv[i++] = token;
                    token = strtok(NULL," ");
                }
                newargv[i] = NULL;
                int error_num = execvp(cmd, newargv);
                
                /* execvp only returns on error */
                printf("%s: command not found\n", cmd);
                status = 1;
                exit(status);
            }else{
                // fork failed
                const char * msg = "PARENT: failed to fork\n";
                write(STDOUT_FILENO, msg, strlen(msg));
                status = 1;
            }  
           
}

int picoshell_main(int argc, char *argv[]) {
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
                write(STDOUT_FILENO, arg, strlen(arg));
            write(STDOUT_FILENO, "\n", 1);
        }   
        else if(strcmp("exit", cmd)==0)
        {
                const char * msg = "Good Bye\n";
                write(STDOUT_FILENO, msg, strlen(msg));
                exit(status);
        }else if(strcmp("cd", cmd)==0)
        {
            cd_cmd();
        }
        else if(strcmp("pwd", cmd)==0)
        {
            pwd_cmd();
        }
        else{
            execute_custom_cmd();
        }
        
    }
    return 0;
}

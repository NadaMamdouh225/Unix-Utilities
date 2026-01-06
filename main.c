#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>



char *cmd;
char *arg;
int status = 0;
int save_fd_in ;
int save_fd_out;
int save_fd_err;

extern char **environ;
typedef struct Variables
{
    char *variable;
    char *value;
    struct Variables *next;
}Variables;

Variables *local_vars = NULL;   // Linked list of local variables
// ---------------------------------------------------------------------------

// Functions declaration
int is_assignment(char *cmd, char *arg);
void extract_var(char* str,int is_env);
char* Substitute_var(char* str);
void set_local_var(char* variable, char* value);
char* get_local_var(char *variable);
void free_local_vars(void);
void set_env_var(char* variable, char* value);
char* get_env_var(char* variable);
char* handle_spaces(char *inBuf);
int getCmd(void);
void pwd_cmd(void);
void cd_cmd(void);
void execute_custom_cmd(void);
void echo_cmd(void);
void exit_cmd(void);
char* handle_redirection(char* str);
void restore_fds(void);

// ---------------------------------------------------------------------------

int is_assignment(char *cmd, char *arg)
{
    char *ptr = strchr(cmd, '=');
    if(ptr != NULL)
        return 1;   // local variable

    if(strcmp(cmd, "export")==0 && arg != NULL)
        return 2;   // environment variable

    return 0;       // there is no '=' sign
}

// Extract local and environmant varibles and store it in the appropriate data structure
void extract_var(char* str,int is_env)
{
    char *copy = strdup(str);
    char *ptr = strchr(copy, '=');

    *ptr = '\0';
    
    int l = strlen(copy);
	if ((ptr+1)[0] ==  ' ' || copy[l-1] ==  ' ' )
    {
        perror("Invalid command\n");
        status = 1;
        return;
    }
		
    char *variable = copy;
    char *value = strtok(ptr +1, " ");

    if(is_env)
    {
        set_env_var(variable, value);
    }
    else
    {
        set_local_var(variable, value);
    }

}
// support $var in command
char* Substitute_var(char* str)
{
    if(str == NULL)
        return NULL;

    char *modified_cmd = (char*)malloc(strlen(str) * 4);
    if (modified_cmd == NULL)
    {
        perror("malloc: failed to allocate");
        status = 1 ;
        return NULL;
    }

    // Looking for $ then replace it
    int i =0, j = 0;
    while (str[i] != '\0')
    {
        if(str[i] == '$')
        {
            i++;    //skip $

            char var_name[256];
            int k = 0;
            
            // Get variable name
            while(str[i] != '\0' && (isalnum(str[i])|| str[i]=='_'))
            {
                var_name[k++] = str[i++];
            }
            var_name[k] = '\0';

            // Get variable value
            char* res = get_env_var(var_name);
            if(res == NULL)
                res = get_local_var(var_name);

            // Modify the command with the value
            if(res!=NULL)
            {
                strcpy(&modified_cmd[j], res);
                j+= strlen(res);
            }

        }else{
           modified_cmd[j++] = str[i++]; 
        }
    }

    modified_cmd[j] = '\0';
    return modified_cmd;    
}

// Add or update local variables
void set_local_var(char* variable, char* value)
{
    Variables *temp = local_vars;
    while (temp != NULL)
    {
        // if value exist then update
        if(strcmp(temp->variable, variable) == 0)
        {
            free(temp->value);
            temp->value = strdup(value);
            return;
        }
        temp = temp->next;
    }
    
    // create new one
    Variables *new_local_var = (Variables*)malloc(sizeof(Variables));
    if(new_local_var == NULL)
        {
            perror("malloc failed");
            status = 1;
            return;
        }

    new_local_var->variable = strdup(variable);
    new_local_var->value    = strdup(value);

    if (!new_local_var->variable || !new_local_var->value) {
        perror("strdup failed");
        free(new_local_var->variable);
        free(new_local_var->value);
        free(new_local_var);
        status = 1;
        return;
    }

    new_local_var->next = local_vars;
    local_vars = new_local_var;

}
//  Get a local variable value
char* get_local_var(char *variable)
{
    Variables *temp = local_vars;
    while(temp != NULL)
    {
        if(strcmp(temp->variable, variable) == 0)
            return temp->value;
        temp = temp->next;
    }
    return NULL;
}
void free_local_vars(void)
{
    Variables *temp = local_vars;
    while(temp != NULL)
    {
       Variables *next = temp->next;
        free(temp->variable);
        free(temp->value);
        free(temp);
        temp = next;
    }

}
void set_env_var(char* variable, char* value)
{
    if(setenv(variable, value, 1) != 0)
    {
        status = 1;
        perror("setenv failed");
    }
}
char* get_env_var(char* variable)
{
    return getenv(variable);
}
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
    static char *outBuffer = NULL;  

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
                // Redirect and return a copy without redirection operators
                char *arg_copy_for_exec = handle_redirection(arg);

                if (arg_copy_for_exec == NULL && arg != NULL) 
                {
                    exit(1);
                }

                // count arguments
                int arg_count = 1;
                if(arg != NULL)
                {
                    char *temp = strdup(arg_copy_for_exec);  // Duplicate to count without modifying
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
                char* token = strtok(arg_copy_for_exec," ");
                while(token != NULL)
                {
                    newargv[i++] = token;
                    token = strtok(NULL," ");
                }
                newargv[i] = NULL;

                execvp(cmd, newargv);
                
                /* execvp only returns on error */
                fprintf(stderr, "%s: command not found\n", cmd);
                if (arg_copy_for_exec) free(arg_copy_for_exec);
                free(newargv);
                exit(1);
            }else{
                // fork failed
                const char * msg = "PARENT: failed to fork\n";
                write(STDOUT_FILENO, msg, strlen(msg));
                status = 1;
            }  
           
}
void echo_cmd(void)
{
    if(arg != NULL)
                write(STDOUT_FILENO, arg, strlen(arg));
    write(STDOUT_FILENO, "\n", 1);
}
void exit_cmd(void)
{
    const char * msg = "Good Bye\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    exit(status);
}

char* handle_redirection(char* str)
{
    
    if(str == NULL)
        return NULL;
    char *cmd_without_redir = (char*)malloc(strlen(str) + 1);
    if (cmd_without_redir == NULL)
    {
        perror("malloc: failed to allocate");
        status = 1 ;
        return NULL;
    }
    int i = 0, j = 0;
    while(str[i]!='\0')
    {
       
        if(str[i] == '>')  // if stdout
        {
            // Remove trailing spaces before the redirection operator
            while(j > 0 && isspace(cmd_without_redir[j-1])) {
                j--;
            }

            i++;
            while (isspace(str[i])) i++;
            
            char file_name[256];
            int k = 0;
        
            // Get file name
            while(str[i]!='\0'&&  !isspace(str[i]))
            {
                file_name[k++] = str[i++];
            }
            file_name[k] = '\0';

            int out_fd ;
            if((out_fd = open(file_name,O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
            {   
                perror(file_name);
                status = 1;
                free(cmd_without_redir);
                return NULL;
            }
            dup2(out_fd, 1);
            close(out_fd);

        }
        else if(str[i] == '2' && str[i+1] == '>')  // if stderr
        {
            // Remove trailing spaces before the redirection operator
            while(j > 0 && isspace(cmd_without_redir[j-1])) {
                j--;
            }

            i+=2;
            while (isspace(str[i])) i++;

            char file_name[256];
            int k = 0;
            // Get file name
            while(str[i]!='\0' &&  !isspace(str[i]))
            {
                file_name[k++] = str[i++];
            }
            file_name[k] = '\0';

            int err_fd ;
            if((err_fd = open(file_name,O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
            {   
                perror(file_name);
                status = 1;
                free(cmd_without_redir);
                return NULL;
            }
            dup2(err_fd, 2);
            close(err_fd);

        }
        else if(str[i] == '<')  // if stdin
        {
            // Remove trailing spaces before the redirection operator
            while(j > 0 && isspace(cmd_without_redir[j-1])) {
                j--;
            }
            
            i++;
            while (isspace(str[i])) i++;
            char file_name[256];
            int k = 0;
            // Get file name
            while(str[i]!='\0'&& !isspace(str[i]))
            {
                file_name[k++] = str[i++];
            }
            file_name[k] = '\0';

            int in_fd ;
            if((in_fd = open(file_name,O_RDONLY, 0644)) < 0)
            {   
                fprintf(stderr, "cannot access %s: No such file or directory\n" ,file_name);
                status = 1;
                free(cmd_without_redir);
                return NULL;
            }
            dup2(in_fd, 0);
            close(in_fd);

        }else{
            cmd_without_redir[j++] = str[i++];
        }
    }
    cmd_without_redir[j] = '\0';
    return cmd_without_redir;
}
void restore_fds(void)
{
    dup2(save_fd_in, STDIN_FILENO);
    dup2(save_fd_out, STDOUT_FILENO);
    dup2(save_fd_err, STDERR_FILENO);
    close(save_fd_in);
    close(save_fd_out);
    close(save_fd_err);
}

int main(int argc, char *argv[]) {
    while(1)
    {
        const char *shellmsg = "Shell prompt > ";
        write(STDOUT_FILENO, shellmsg, strlen(shellmsg));

        if(getCmd() == 0)
        {
            free_local_vars();
            exit(status);
        }
        
         // handle empty line 
        if (cmd == NULL)
            continue;

        
        int check_assignment = is_assignment(cmd, arg);
        if(check_assignment == 1)
        {
            extract_var(cmd, 0);
            continue;
        }
        else if(check_assignment == 2)
        {
            // export VAR=value
            if (strchr(arg, '='))
            {
                extract_var(arg, 1);
            }
            else
            {
                // export Var (from local to env)
                char* val = get_local_var(arg);
                if (val != NULL)
                {
                    set_env_var(arg, val);
                }
                else
                {
                    fprintf(stderr, "export: %s: not found\n", arg);
                    status = 1;
                }
            }
            continue;
        }

        char *expanded_cmd = NULL;
        char *expanded_arg = NULL;

        /* Expand command */
        expanded_cmd = Substitute_var(cmd);
        if (expanded_cmd != NULL)
        {
            cmd = expanded_cmd;
        }

        /* Expand arguments */
        if (arg != NULL)
        {
            expanded_arg = Substitute_var(arg);
            if (expanded_arg != NULL)
            {
                arg = expanded_arg;
            }
        }
        
        if(strcmp("echo", cmd)==0 || strcmp("cd", cmd)==0 || strcmp("pwd", cmd)==0)
        {
            // Save fd 
            save_fd_in = dup(STDIN_FILENO);
            save_fd_out = dup(STDOUT_FILENO);
            save_fd_err = dup(STDERR_FILENO);

            // Redirect and remove redirection operator
            char* arg_without_redir = handle_redirection(arg);
            if (status != 0)
            {
                restore_fds();
                if (arg_without_redir) free(arg_without_redir);
                continue;
            }else{
                arg = arg_without_redir;
            }
            

            if(strcmp("echo", cmd)==0) 
            {
                echo_cmd();
            }   
            else if(strcmp("cd", cmd)==0)
            {
                cd_cmd();
            }
            else if(strcmp("pwd", cmd)==0)
            {
                pwd_cmd();
            }

            // Restore fd 
            restore_fds();
        }
        else if(strcmp("exit", cmd)==0)
        {
            if(expanded_cmd) free(expanded_cmd); 
            if(expanded_arg) free(expanded_arg); 
            exit_cmd();
        }
        else{
            execute_custom_cmd();
        }

        if(expanded_cmd) free(expanded_cmd); 
        if(expanded_arg) free(expanded_arg); 
    }
    free_local_vars();
    return 0;
}

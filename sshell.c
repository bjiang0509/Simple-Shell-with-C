#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>

#define CMDLINE_MAX 512
#define ARGS_LENGTH_MAX 18

struct command {
        char cmd[CMDLINE_MAX];
        char* execName; 
        char **execArgs;
    /* first_cmd store the command before > sign 
            and fileName stores the file name for output redirection*/
        char first_cmd[CMDLINE_MAX]; 
        char* fileName; 
};

void parse(char *commandInput, struct command *cmd) {
    /*this function parse the command line and store it in
            the struct variable command cmd*/
        char *token;
        int index = 0;

        token = strtok(commandInput, " ");
        cmd->execName = token;
        while (token != NULL) {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    break;
                }
                strcpy(cmd->execArgs[index],token);
                index++;
        }

    return;
}

void RemoveExtraWhiteSpace(char mod_str[], char ori_str[]){
        char* token;
        mod_str[0] = '\0';
        token = strtok(ori_str, " ");

        while(token != NULL){
                strcat(mod_str, token);
                token = strtok(NULL, " ");
                if(token != NULL){
                        strcat(mod_str, " ");
                }
        }
        return;
}

void separateStrings(char cmd[], char first_cmd[], char** fileName, char* c, char* s){
    /* separates cmd into 2 parts based on the delimiter c */
        char* break_cmd;
        break_cmd = strtok(cmd, c);
        int i = 0;
        while(break_cmd != NULL){
                if(i == 0){
                        strcpy(first_cmd, break_cmd);
                        ++i;
                }else{
                        strcpy(*fileName, break_cmd);
                }
                break_cmd = strtok(NULL, s);
        }
}

long int getSize(char file_name[]){
    /* set pointer to beginning of file then fseek to the end. */
        FILE* file = fopen(file_name, "r");
        fseek(file, 0, SEEK_SET);
        fseek(file, 0, SEEK_END);
        /*Use ftell to determine current location in file. The location
                at the end of the file is the file size*/
        long int file_size = ftell(file);
        fclose(file);
        return file_size;
}

void outputRedirect(char* file_name, bool output_append){
    /* If output_append is false, the function overwrites the file with the output
            of executions. Else, the output will be appended to the file with flag O_APPEND */
        int fd; 

        if(output_append == false){
                fd = open(file_name, O_CREAT|O_RDWR, S_IRWXU); //permissions; reference: https://linux.die.net/man/3/open

                if (fd == -1)
                {
                        fprintf(stderr, "Error: missing command\n");
                        exit(1);
                }
                dup2(fd, STDOUT_FILENO);
        }else{
                fd = open(file_name, O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
                if (fd == -1)
                {
                        fprintf(stderr, "Error: missing command\n");
                        exit(1);
                }
                dup2(fd, STDOUT_FILENO);
        }
        close(fd);
        return;

}

/*piping*/

void pipeline_cmd(struct command cmds[], int num_pipe, char cmd[], bool output_append, bool output_red, char* file_name){

        int status;
        int i = 0;
        int fd[num_pipe][2];
        int num_process = num_pipe + 1;
        int stat[num_process];
        pid_t pid[num_process];
        
        /*create num_pipe numbers of pipes*/
        for(int l = 0; l < num_pipe; ++l){
                pipe(fd[l]);
        }

        /*while i < num_process, fork() is called to create child process. */
        /*reference: video on one pipe: https://youtu.be/Mqb2dVRe0uo */
        while(i < num_process){
                pid[i] = fork();
                if(pid[i] == 0){
                        char *exe_args[ARGS_LENGTH_MAX];
                        int j = 1;
                        if(i == 0){
                        /* If the child process is the first process on command line, its output
                                will be write to the first pipe. */
                                dup2(fd[i][1],STDOUT_FILENO);
                                close(fd[i][1]);
                        }else if(i == num_pipe){
                        /* If it's the last process, then we close the write end of the previous
                                pipe and read from the previous pipe (should contain the output of the 
                                previous process)*/
                                close(fd[i-1][1]);
                                dup2(fd[i-1][0], STDIN_FILENO);
                        }else{
                        /* If current process is not the first or last process, then we read from
                                the previous pipe and output to next pipe*/
                                close(fd[i-1][1]);
                                dup2(fd[i-1][0], STDIN_FILENO);
                                dup2(fd[i][1], STDOUT_FILENO);
                                close(fd[i][0]);
                        }

                        exe_args[0] = cmds[i].execName;
                        while(j < ARGS_LENGTH_MAX){
                                if(!strcmp(cmds[i].execArgs[j-1], "\0")){
                                        break;
                                }  
                                exe_args[j] = cmds[i].execArgs[j-1];
                                ++j;   
                        }
                        exe_args[j] = NULL;

                        if (output_red && i == num_pipe){outputRedirect(file_name, output_append);}
                        execvp(cmds[i].execName, exe_args);
                        perror("fork loop ");
                        exit(0);
                }else if(pid[i] < 0){
                        perror("pid ");
                        exit(1);
                }
                        /*If pipes not closed, the child process won't terminate*/
                if (i > 0){
                        close(fd[i-1][0]);
                        close(fd[i-1][1]);
                }   
                ++i;
        }

        /* parent wait for all child process to terminate */
        for(int i = 0; i <= num_pipe; ++i){
                waitpid(pid[i], &status, 0);
                if(WIFEXITED(status)){
                        stat[i] = WEXITSTATUS(status);
                }
        }
        /* print completion message and exit status */
        fprintf(stderr, "+ completed '%s' ", cmd);
        for (int i = 0; i <= num_pipe; ++i)
        {
                fprintf(stderr, "[%d]", stat[i]);
        }
        fprintf(stderr, "\n");
        return;
}

void RegularCommands(struct command cmd_str, char* file_name, bool output_append, bool output_red){
    /* If command involves external programs, this function will be called*/
        pid_t pid;
        char *exe_args[ARGS_LENGTH_MAX];
        int i = 1;

        exe_args[0] = cmd_str.execName;
        while(i < ARGS_LENGTH_MAX){
                if(!strcmp(cmd_str.execArgs[i-1], "\0")){
                    break;
                }
                exe_args[i] = cmd_str.execArgs[i-1];
                ++i; 
                if (i > 16)
                {
                    fprintf(stderr, "Error: too many process arguments\n");
                    return;
                }  
        }
        exe_args[i] = NULL;

        pid = fork();
        if(pid == 0){
                //child
                if (output_red){outputRedirect(file_name, output_append);}
                execvp(cmd_str.execName,exe_args);
                fprintf(stderr, "Error: command not found\n");
                exit(1);
        }
        else if(pid > 0){
                //parent
                int status; 
                waitpid(pid, &status, 0);
                fprintf(stderr,"+ completed '%s': [%d]\n", cmd_str.cmd, WEXITSTATUS(status));
        }else{
                perror("fork");
                exit(1);
        }

        return;
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char cmd2[CMDLINE_MAX];
        char cwd[PATH_MAX];

        while (1) {
                char *nl;
                struct command cmd_str;
                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Copy cmd to cmd_str.cmd for printing completion message*/
                strcpy(cmd_str.cmd, cmd);
                strcpy(cmd2, cmd);
                RemoveExtraWhiteSpace(cmd, cmd2);

                /*Check if output should be redirect or append to another file */
                bool output_append = false;
                bool output_red = false;
                //int i = 0;
                nl = strchr(cmd, '>');
                if(nl != NULL){
                        output_red = true;
                }

                char* n2 = strrchr(cmd, '>');
                if(n2 != NULL && nl != n2){
                        output_append = true;
                }

                /* check if missing commands for pipe */
                char check_pipe_command[CMDLINE_MAX];
                char before_pipe[CMDLINE_MAX];
                char* after_pipe = calloc(CMDLINE_MAX, sizeof(char));
                strcpy(check_pipe_command,cmd);
                nl = strrchr(check_pipe_command, '|');
                if(nl){
                        separateStrings(check_pipe_command, before_pipe, &after_pipe, "|", "|");
                        if(!strcmp(after_pipe, "\0")){
                                fprintf(stderr, "Error: missing command\n");
                                continue;
                        }
                }
                free(after_pipe);

                /* If the first charater in command line is > or |, then error message
                        will be printer*/
                char cmd_copy[CMDLINE_MAX];
                strcpy(cmd_copy, cmd);
                bool miss_command = false;
                for(int i = 0; i < CMDLINE_MAX; ++i){
                        if(isalpha(cmd_copy[i])){
                                break;
                        }
                        if(cmd_copy[i] == '>' || cmd_copy[i] == '|'){
                                fprintf(stderr, "Error: missing command\n");
                                miss_command = true;
                                break;
                        }
                }
                if(miss_command == true){
                    continue;
                }
                /* Use calloc to allocate space for double array containing
                        to executable*/
                cmd_str.execArgs = (char**) calloc(ARGS_LENGTH_MAX, sizeof(char*));
                for(int i = 0; i < ARGS_LENGTH_MAX; ++i){
                        cmd_str.execArgs[i] = (char*) calloc(CMDLINE_MAX, sizeof(char*));
                }

                cmd_str.fileName = calloc(CMDLINE_MAX, sizeof(char));

                char* file_name;
                if(output_red == true){
                        /* If output redirection is needed, cmd will be split into two at the > symbol. The second part
                                is the file name*/

                        separateStrings(cmd, cmd_str.first_cmd, &cmd_str.fileName, ">",">");

                        /* If pipe symbol appears after >, then print error message*/
                        nl = strchr(cmd_str.fileName, '|');
                        if(nl != NULL){
                            fprintf(stderr, "Error: mislocated output redirection\n");
                            continue;
                        }

                        /* If there's nothing after >, then print error message*/
                        if(!strcmp(cmd_str.fileName, "\0")){
                            fprintf(stderr, "Error: no output file\n");
                            continue;
                        }

                        /* If / symbol is in filename, we cannot access it and print error message */
                        file_name = strtok(cmd_str.fileName, " "); 
                        nl = strchr(file_name, '/');
                        if(nl != NULL){
                            fprintf(stderr, "Error: cannot open output file\n");
                            continue;
                        } 
                                
                }

                /* Check to see if piping is required and count the number of pipes needed*/
                bool piping = false;
                char* pipeline_locator;
                int pipeline_counter = 0;
                pipeline_locator = strchr(cmd, '|');
                if(pipeline_locator != NULL){
                        piping = true;
                        for(unsigned i = 0; i < strlen(cmd);++i){
                                if(cmd[i] == '|'){
                                        ++pipeline_counter;
                                }
                        }
                }

                /* If piping == true, we create a array pipe_cmd of struct type command 
                        and stored each process in it*/
                if(piping == true){
                        struct command pipe_cmd[pipeline_counter];
                        int count = 0;
                        if(output_red == true){
                                char* token = strtok(cmd_str.first_cmd, "|");
                                while(token != NULL){
                                        strcpy(pipe_cmd[count].cmd, token);
                                        ++count;
                                        token = strtok(NULL, "|");
                                } 
                        }else{
                                char* token = strtok(cmd, "|");
                                while(token != NULL){
                                        strcpy(pipe_cmd[count].cmd, token);
                                        ++count;
                                        token = strtok(NULL, "|");
                                } 
                        }
                        
                        /*Allocate space for each process*/
                        for(int i = 0; i <= pipeline_counter ; ++i){
                                pipe_cmd[i].execArgs = (char**) calloc(ARGS_LENGTH_MAX, sizeof(char*));
                                for(int j = 0; j < ARGS_LENGTH_MAX; ++j){
                                        pipe_cmd[i].execArgs[j] = (char*) calloc(CMDLINE_MAX, sizeof(char*));
                                }
                        }


                        /* Parse each process and remove extra whitespaces */
                        for(int i = 0; i <= pipeline_counter; ++i){
                                parse(pipe_cmd[i].cmd, &pipe_cmd[i]);
                                char valid_command[CMDLINE_MAX];
                                RemoveExtraWhiteSpace(valid_command, pipe_cmd[i].execName);
                                strcpy(pipe_cmd[i].execName, valid_command);
                        }

                        /* Calls the pipeline_cmd function*/
                        pipeline_cmd(pipe_cmd, pipeline_counter, cmd_str.cmd, output_append, output_red, file_name);

                        /* Free all spaces used by the processes */
                        for(int i = 0; i <= pipeline_counter; ++i){
                                for(int j = 0; j < ARGS_LENGTH_MAX; ++j){
                                        free(pipe_cmd[i].execArgs[j]);
                                }
                                free(pipe_cmd[i].execArgs);
                        }
                        continue;
                }

                /*If piping is not invloved, we parse the cmd for built in commands or regular commands*/ 
                if(output_red == true){
                        parse(cmd_str.first_cmd, &cmd_str);
                }else{
                        parse(cmd,&cmd_str);
                }

                /* Builtin command */
                if (!strcmp(cmd_str.execName, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed 'exit' [0]\n");
                        break;
                }
                if(!strcmp(cmd_str.execName, "pwd") || !strcmp(cmd_str.execName,"cd")
                        || !strcmp(cmd_str.execName,"sls")){
                        int exit_stat = 0;
                        char* dir_path = getcwd(cwd, sizeof(cwd));

                        /* Pwd built in command */
                        if(!strcmp(cmd_str.execName, "pwd")) {
                                fprintf(stdout, "%s\n", dir_path);
                        }

                        /* Cd built in command*/
                        if(!strcmp(cmd_str.execName,"cd")) {
                                if(chdir(cmd_str.execArgs[0]) == 0){
                                        if(!strcmp(cmd_str.execArgs[0], "..")){
                                                chdir(".");
                                        }else{
                                                chdir(cmd_str.execArgs[0]);
                                        }
                                }
                                else{
                                        fprintf(stderr, "Error: cannot cd into directory\n");
                                        exit_stat = 1;
                                }
                                
                        }

                        /* Sls built in command */
                        DIR *dp;
                        struct dirent *ep;
                        if(!strcmp(cmd_str.execName,"sls")){
                                dp = opendir(dir_path);
                                if(dp != NULL){
                                        while((ep = readdir(dp)) != NULL){
                                                long int file_size = getSize(ep->d_name);
                                                if(!strcmp(ep->d_name,".") || !strcmp(ep->d_name, "..")){
                                                        continue;
                                                }
                                                if(file_size != 0){
                                                        printf("%s ", ep->d_name);
                                                        printf("(%ld bytes)", file_size);
                                                        printf("\n");
                                                }else{
                                                        printf("%s ", ep->d_name);
                                                        printf("(0 bytes)");
                                                        printf("\n");
                                                }
                                        }
                                        (void) closedir(dp);
                                }else{
                                        perror("Error ");
                                        exit_stat = 1;
                                }

                        }

                        /* Print completion message for built in commands*/
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd_str.cmd, exit_stat);
                        continue;
                }

                /*Call RegularCommands to launch external executables*/
                RegularCommands(cmd_str, file_name, output_append, output_red);
                
                /*free all spaces used by executable arguments*/
                free(cmd_str.fileName);
                for (int i = 0; i < ARGS_LENGTH_MAX; ++i )
                {
                        free(cmd_str.execArgs[i]);
                }
                free(cmd_str.execArgs);
        }

        return EXIT_SUCCESS;
}

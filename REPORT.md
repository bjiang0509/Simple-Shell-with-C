# ECS 150 FQ 2021, Project 1: Simple Shell Report

## Authors
Name: Bijun Jiang, Student ID: 914728892
Name: 

## High-Level Design Choices
We utilized the struct feature and developed multiple helper functions in the implementation of our simple shell. 
The struct we created is called "command." The command struct contains an char array variable that holds the complete command from the command line, an pointer (char* execName) to the executable name, an double pointer (char** execArgs) that points to the individual arguments for the executable, and two char arrays(char first_cmd[CMDLINE_MAX] and char second_cmd[CMDLINE_MAX]) to store the commands before the redirection symbol ">" and the file name located after ">".
In order to increase the flexibilty, manageability, and simplicity of our codes, we decided to create functions for different purposes. The functions and their uses are as follows:
void parse(char *commandInput, struct command *cmd) : take commandInput and parse it into execuable and arguments, then store it in the struct variable cmd. The pointer to the cmd allow us to change the values of the struct directly.
void RemoveExtraWhiteSpace(char mod_str[], char ori_str[]): removes extra white spaces in ori_str and store in mod_str.
void separateStrings(char cmd[], char first_cmd[], char fileName[], char* c, char* s): separates cmd into two parts (first_cmd and fileName) base on the delimiters c and s.
long int getSize(char file_name[]): returns the file size of the file(file_name[]) by using ftell after fseek to the end of the file.
void outputRedirect(char* file_name, bool output_append): redirects STDOUT_FILENO to the file indicated by file_name. Overwrites or appends the output base on the value of output_append.
void pipeline_cmd(struct command cmds[], int num_pipe, char cmd[], bool output_append, bool output_red, char* file_name): create num_pipe number of pipes and run num_pipe+1 processes by utilizing fork in a while loop.
void RegularCommands(struct command cmd_str, char* file_name, bool output_append, bool output_red): carries out regular commands that are not built in and commands that does not involve piping.

## Implementation Details
### In Main
In main, we checked for the ">" and "|" symbols using strchr(return pointer to first occurrence). If the pointer checking for ">" and "|" is not null, then the bool variables output_red and piping will be set to true. Then, we used strrchr to locate the last occurence of ">". If the location of first occurrence and the last occurence for ">" are different, then output_append will be set to true.
If piping is true, we call the pipeline_cmd function and continues. If piping is true, we check for built in commands. If the executable is not a built in command, we call the RegularCommands function.

### Parsing
Our parsing function is dependent on the strtok() function defined in the GNU library. The delimiter for parsing is a white space " ". The first token is copied (using strcpy) to execName in the struct command varaible cmd_str. The tokens before encountering NULL are copied to the execArgs double array. It is important to note that execArgs is defined as a double pointer in the struct command. So we allocated space for it in main by calling calloc before calling parse on the struct command variable.

### Built in Commands
We decided to keep our built in commands in main to remind ourselves that built in commands should be done by the shell. If execName is "pwd", "cd", or "sls", the int exit_stat (initialize to 0) and char* dir_path(initialize with the current working directory path obtained by getcwd) will be created. "pwd" is implemented by printing dir_path. "cd" is implemented by chdir(cmd_str.execArg[0]), because the first argument for cd is the directory name. Lastly, "sls" requires DIR* dp and struct dirent* ep. dp opens the current directory with opendir. While dp != NULL, we print ep->d_name and the file size by calling the getSize function.

### Regular Commands
Regular commands requires launching external programs. When the function is called, char* exe_arg[ARGS_LENGTH_MAX] will be created. Its first argument will be set to the executable (execName). And the arguments to the executable (stored in execArgs) will be added to exe_arg through a while loop. The last element for exe_arg is NULL. Next, we fork() and child process is created. We use execvp(cmd_str.execName, exe_arg) to launch the external process. The parent process (the shell itself) waits for the child process, prints completion message, and return to wait for the next command.

### Output Redirection
If output_red is true and output_append is false, the function will open the file passed through file_name with O_CREAT and O_RDWR flags in (S_IRWXU = 00700) mode. If output_append is true, the file will be opened with the addition of the O_APPEND flag for appending. Then, we direct the output from STDOUT_FILENO to fd (the file descriptor of the open file).

### Pipeline Commands
Recall that we counted the number of pipes in main. We pass that number to the pipeline function and create a two-dimensional array int fd[num_pipe][2]. Using a forloop and pipe(), we create num_pipe of pipes. 
Next, while the counter i is less than the number of processes, we will create a child process with fork(). If the child process is the first process, we write its output to pipe 1 (fd[1][1]). If the child process is the last process, we read its input from the previous pipe (fd[i-1][0]) and output to STDOUT. If the process is not the first and is not the last process, then it'll read from the previous pipe for input and write its output to the current pipe. We launch the execution process of each child the same way as regular commands. 

## Testing
The first step in our testing process is to run the provided tester.sh. Once we passed all the test cases in the partial autograder, we test individual cases in the prompt and compare outputs from shell_ref with the diff command.

## Sources
1.https://linux.die.net/man/3/open
2.https://youtu.be/Mqb2dVRe0uo
3.https://man7.org/linux/man-pages/man2/open.2.html
4.https://www.gnu.org/software/libc/manual/html_mono/libc.html#Working-Directory
5.Lecture on syscalls and the PDF 03.syscall
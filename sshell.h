#ifndef SSHELL_H_   /* Include guard */
#define SSHELL_H_

#define CMDLINE_MAX 512
struct CmdArgs
{
	char cmd[CMDLINE_MAX];
	char exe[CMDLINE_MAX];
	char first_cmd[CMDLINE_MAX];
	char second_cmd[CMDLINE_MAX];
	char arguments[16][CMDLINE_MAX];
	char args[CMDLINE_MAX];
};

long int getSize(char file_mdname[]);
void separateStrings(char cmd[], char exe[], char args[], char* c, char* s);
//void builtInCmd(struct CmdArgs cmd_str, char* cwd);
void RegularCommands(char cmd[], char exe[], char args[], bool output_red, bool output_append, char* file_name);
void outputRedirect(char* file_name, bool output_append);
void pipeline_cmd(struct CmdArgs cmds[], int num_pipe, char cmd[]);

#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAXARGS 10

/* Parse input command */ 
/* return 1 if background process */
/* return 0 if foreground process */

int parse_line(char *cmd, char **argv)
{
	int bg;			/* background or foreground process */
	int argc;		/* number of arguments to be passed to exec */
	char *delim;	/* points to first space occurence in cmd */

	/* Ignore leading spaces in cmd */
	while(*cmd && (*cmd == ' '))
	{
		cmd++;
	}

	/* Build argv list */
	argc = 0;
	while((delim = strchr(cmd, ' ')))
	{
		argv[argc++] = cmd;
		*delim = '\0';	/* To NULL terminate the token pointer */
		printf("%s\n", cmd);
		cmd = ++delim;
		/* Ignore leading spaces in cmd after token */
		while(*cmd && (*cmd == ' '))
		{
			cmd++;
		}
	}
	/* argv is NULL terminated */
	argv[argc] = NULL;

	/* Blank command */
	if(argc == 0)
	{
		return 1;
	}

	/* If background process, return 1 and truncate & from cmd */
	if((bg = (*argv[argc - 1]) == '&'))
	{
		argv[--argc] = NULL;
	}

	return bg;
}



int main()
{
	char cwd[PATH_MAX];	/* current directory */
	char *cmd;
	char *argv[MAXARGS];

	while(1) 
	{
		if (getcwd(cwd, sizeof(cwd)) != NULL) 
		{
			printf("%s", cwd);
			cmd = readline(": ");
			cmd = (char*)realloc(cmd, sizeof(char)*strlen(cmd));
			/* last character is set to ' ' to parse properly */
			cmd[strlen(cmd)] = ' ';
			parse_line(cmd, argv);
			free(cmd);
		} 
		else 
		{
			printf("error\n");
			return 1;
		}
	}

	return 0;
}

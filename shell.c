#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
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
		*delim = '\0';	/* To NULL terminate the token pointer */
		argv[argc++] = cmd;
		cmd = ++delim;
		/* Ignore leading spaces in cmd after token */
		while(*cmd && (*cmd == ' '))
		{
			cmd++;
		}
	}
	/* argv is NULL terminated */
	argv[argc] = NULL;

	int i = 0;

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

int builtin_cmd(char **argv)
{
	if (!strcmp(argv[0], "exit")) /* quit command */
		exit(0);
	if (!strcmp(argv[0], "&"))
		return 1;				/* Ignore singleton & */
	
	return 0;
}

int main()
{
	char cwd[PATH_MAX];	/* current directory */
	char *cmd;
	char *argv[MAXARGS];
	char *environ[] = { NULL };
	int i = 0;
	pid_t pid;
	int status, bg;

	while(1) 
	{
		if (getcwd(cwd, sizeof(cwd)) != NULL) 
		{
			/* Print prompt */
			printf("%s", cwd);
		} 
		else 
		{
			fprintf(stderr, "cwd error: %s\n", strerror(errno));
			return 1;
		}	

		/* Read command */
		cmd = readline(": ");
		cmd = (char*)realloc(cmd, sizeof(char)*strlen(cmd));
		/* last character is set to ' ' to parse properly */
		cmd[strlen(cmd)] = ' ';

		/* Tokenize command */
		bg = parse_line(cmd, argv);

		/* Evaluate command */
		/* Ignore blank command */
		if(argv[0] == NULL)
			return 1;

		/* Fork and exec if not builtin command */
		if(!builtin_cmd(argv))
		{
			/* Create child process to run command */
			pid = fork();

			if(pid < 0)
			{
				fprintf(stderr, "fork error: %s\n", strerror(errno));
			}
			else if(pid == 0)
			{
				/* Load image of executable in child process */
				if(execvp(argv[0], argv) < 0)
				{
					/* execvp does not return if successful */
					fprintf(stderr, "execve error: %s\n", strerror(errno));
					free(cmd);
					return 1;
				}
			}
			else
			{
				/* Shell waits for foreground process to terminate */
				if(!bg)
				{
					if(waitpid(pid, &status, 0) < 0)
					{
						fprintf(stderr, "waitpid error: %s\n", strerror(errno));
						free(cmd);
						return 1;
					}
				}
			}
		}
	}

	return 0;
}

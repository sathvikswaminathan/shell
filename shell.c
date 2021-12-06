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
#define MAXJOBS 10

/* Parse input command */ 
/* return 1 if background process */
/* return 0 if foreground process */

char *cmd;				/* Input from user */

struct job
{
	pid_t pid;			/* Process ID of job */
	char *cmd;			/* Command name */
	struct job *next;	/* Pointer to next job */
};

struct job* head = NULL;

void print_job(struct job*, pid_t);
void delete_job(struct job**, pid_t);

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

/* returns 1 if built-in command */
/* exit, &, cd, */
int builtin_cmd(char **argv)
{
	if (!strcmp(argv[0], "exit")) /* exit command */
	{
		free(cmd);
		exit(0);
	}

	if (!strcmp(argv[0], "&"))
		return 1;				/* Ignore singleton & */

	if(!strcmp(argv[0], "cd"))
	{
		if(chdir(argv[1]) < 0)
		{
			fprintf(stderr, "cd error: %s\n", strerror(errno));
			return 1;
		}
		return 1;
	}

	if(!strcmp(argv[0], "history"))
	{
	    HISTORY_STATE *hist = history_get_history_state();
		HIST_ENTRY **list = history_list();

	    printf ("Session history: \n");
	    for (int i = 0; i < hist->length; i++) 
	    { 
	        printf (" %8s  %s\n", list[i]->line, list[i]->timestamp);
	        free_history_entry(list[i]);     
	    }
	    free(hist);  
	    free(list);
	    return 1;
	}

	if(!strcmp(argv[0], "help"))
	{
		printf("\nList of commands supported: \n\n"
			       "exit: ends shell session\n"  
			       "cd [path]: changes directory\n"  
			       "history: displays history of commands\n"
			       "kill [pid]: terminates process\n"
			       "jobs: lists currently running jobs\n"   
		       );
		printf("\nRun jobs in foreground or background(&)\n\n");
		return 1;
	}

	if(!strcmp(argv[0], "kill"))
	{
		kill(atoi(argv[1]), SIGKILL);
		return 1;
	}

	if(!strcmp(argv[0], "jobs"))
	{
		print_job(head, -1);
		return 1;
	}

	return 0;
}

void sigchld_handler()
{
	sigset_t prev_all;
	pid_t pid;

	while ((pid = waitpid(-1, NULL, 0)) > 0) 
	{
		/* Delete job from job list */
		delete_job(&head, pid);
	}
}

/* Add job to job list */
void add_job(struct job** head, pid_t pid, char* cmd)
{
    struct job* new_job = (struct job*) malloc(sizeof(struct job));
    new_job->pid  = pid;
    new_job->cmd = cmd;
    new_job->next = (*head);
    (*head) = new_job;
    return ;
}

/* Delete job from job list */
void delete_job(struct job** head, pid_t pid)
{
    struct job *temp = *head, *prev;

    if (temp != NULL && temp->pid == pid) {
        *head = temp->next; // Changed head
        free(temp); // free old head
        return;
    }

    while (temp != NULL && temp->pid != pid) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL)
        return;

    prev->next = temp->next;
    free(temp); 
    return;
}

/* Display jobs */
void print_job(struct job* node, pid_t pid)
{
	if(pid < 0)
	{
		/* print all jobs */
		while (node != NULL) 
		{
	        printf("%d:  %s\n", node->pid, node->cmd);
	        node = node->next;
    	}
	}
	else
	{
		/* print one job */
		while (node != NULL) 
		{
			if(node->pid == pid)
			{
				printf("%d:  %s\n", node->pid, node->cmd);
	        	node = node->next;
			}
    	}
	}
	return;
}


int main()
{
	char cwd[PATH_MAX];	/* current directory */
	char *argv[MAXARGS];/* arg vector to be passed to execvp */
	pid_t pid;
	int status, bg;
	sigset_t mask_one, prev_one;

	while(1) 
	{
		/* Install SIGCHLD handler */
		if(signal(SIGCHLD, sigchld_handler) == SIG_ERR)
		{
			printf("SIGCHLD error.\n");
			return 1;
		}

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
		cmd = readline("$ ");
		add_history(cmd);
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
			sigprocmask(SIG_BLOCK, &mask_one, &prev_one); /* Block SIGCHLD */
			/* Create child process to run command */
			pid = fork();

			if(pid < 0)
			{
				fprintf(stderr, "fork error: %s\n", strerror(errno));
			}
			else if(pid == 0)
			{
				sigprocmask(SIG_SETMASK, &prev_one, NULL); /* Unblock SIGCHLD */
				/* Load image of executable in child process */
				if(execvp(argv[0], argv) < 0)
				{
					/* execvp does not return if successful */
					fprintf(stderr, "execvp error: %s\n", strerror(errno));
					free(cmd);
					return 1;
				}
			}
			else
			{
				sigprocmask(SIG_SETMASK, &prev_one, NULL); /* Unblock SIGCHLD */
				add_job(&head, pid, cmd);
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

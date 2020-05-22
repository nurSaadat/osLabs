/*i
**	This program is a very simple shell that only handles 
**	single word commands (no arguments).
**	Type "quit" to quit.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


#define CMDLEN 256
#define MAXARGS 100

int main()
{
	int pid;
	int status;
	int i, fd;
	char command[CMDLEN];

	printf( "[INFO] Program begins.\n" );
	
	for (;;)
	{
		// print the prompt
		printf( "smsh%% 	" );

		// get the command
		fgets( command, CMDLEN, stdin );

		// remove '\n' from the end
		command[strlen(command)-1] = '\0';

		// raise error if too many characters
		if (strlen(command) > 254) 
		{
			printf("[ERROR] The command's length shouldn't exceed 254 characters\n");
		}
		
		int background_run = 0;
		int insert = 0;
		int replace = 0;
		
		// check whether to run it in background
		if (command[strlen(command)-1] == '&')
		{
			background_run = 1;
			command[strlen(command)-1] = '\0';
		}

		// quit command 
		if ( strcmp(command, "quit") == 0 )
			break;

		// fork process
		pid = fork();
		if ( pid < 0 )
		{
			printf( "[ERROR] Error in fork.\n" );
			exit(-1);
		}

		// check whether it is a parent or a child process
		if ( pid != 0 )
		{
			// printf( "[INFO] PARENT. pid = %d, mypid = %d.\n", pid, getpid() );
			// wait if not a background process 
			if (!background_run)
				waitpid( pid, &status, 0 );
		}
		else
		{
			// printf( "[INFO] CHILD. pid = %d, mypid = %d.\n", pid, getpid() );
			
			// create arguments array
			char *argv[MAXARGS];
			argv[0] = strtok(command, " ");
			int i = 1;
			
			while (argv[i] != NULL)
			{
				argv[i] = strtok(NULL, " ");
				i++;
			}
			
			// clean array
			char *cmd[MAXARGS];
			i = 0;
			while (argv[i] != NULL)
			{
				cmd[i] = argv[i];
				i++;
			}	

			if (i >= 100)
			{
				printf("[ERROR] The number of arguments must be less than 100.\n");
				return -1;
			}			
			
			// close the array
			cmd[i] = NULL;
		
			int dstf;

			// check redirection
			if (strcmp(cmd[i-2], ">") == 0)
			{
				replace = 1;
				dstf = open(cmd[i - 1], O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG);
			}

			if (strcmp(cmd[i-2], ">>") == 0)
			{
				insert = 1;
				dstf = open(cmd[i - 1], O_APPEND | O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG);
			}			

			if (insert == 1 || replace == 1)
			{
				if (dstf < 0)
				{
					printf("[ERROR] %s cannot be opened", cmd[i - 1]);
					return -1;				
				}
				dup2(dstf, 1);
				cmd[i-2] = NULL;
				cmd[i-1] = NULL;
			}
			
			// printf("i-2 %s\n i-1 %s\n", cmd[i-2], cmd[i-1]);
			
			// execute child process
			execvp( argv[0], cmd);
			
			if (execvp(argv[0], cmd) == -1)
    			{
				// raise error 'no such program'
				printf("[ERROR] No such program %s\n", argv[0]);
				return -1;
			}
		}
	}
}

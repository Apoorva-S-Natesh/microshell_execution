#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

int	main(void)
{
	int	fd[2];
	int	pid2;
	int	pid1;

	if (pipe(fd) == 1)
		return (1);
	pid1 = fork(); // creating first process
	if (pid1 < 0)
		return (1);
	if (pid1 == 0) //child process 1 ping
	{
		//Reroute the std output of the process that starts ping
		dup2(fd[1], STDOUT_FILENO); // duplicates first fd(fd[1]) into second fd(STDOUT), so STDOUT is going to point to fd[1]
		close(fd[0]);
		close(fd[1]);
		int err = (execlp("ping", "ping", "-c", "2", "google.com", NULL) == -1); // dosnt need the 
		//input to be an array and get access to bash env variables
		//if it cant find the executable to execute, returns an error code.
		if (err == -1)
		{
			printf("Could not find the program to execute \n");
			return (2);
		}
	}
	pid2 = fork();
	if (pid2 < 0)
		return (2);
	if (pid2 == 0) //creating child process 2
	{
		// reroute the std input of the proces that starts grep
		dup2(fd[0], STDIN_FILENO); 
		close(fd[0]);
		close(fd[1]);
		int err1 = execlp("grep", "grep", "rtts", NULL);
		if (err1 == -1)
		{
			printf("Could not find the program to execute \n");
			return (2);
		}
	}
	close(fd[0]);
	close(fd[1]);
	int wstatus;
	wait(&wstatus);
	if (WIFEXITED(wstatus))
	{
		int statuscode = WEXITSTATUS(wstatus);
		if(statuscode == 0)
			printf("Success!\n");
		else
			printf("Failure with status code %d \n", statuscode);
	}
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);
	return (1);
}

//fd[0] is the read end, fd[1] write end of pipe]
// we need to take the standard output from ping to write to the pipe we pass (fd[1], STDOUT_FILENO)
// grep is waiting to read from the pipe , so read from the reading end fd[0]

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h> // for strlen, strcpy later to be replaced with libft functions 
#include <stdlib.h>

#define MAXLT 1024 //max number of letters supported
#define MAXCOM 100 // max number of commands to be supported

void	init_shell();
void	printDir();
int		take_input(char *str);
char	*ft_strsep(char **stringp, const char *delim);
int		parse_pipe(char *str, char **strpiped);
void	parse_space(char *str, char **parsed);
void	exec_args(char **parsed);
void	exec_args_piped(char **parsed, int num_pipes);
int		parse_input(char *str, char **parsed);

void	init_shell()
{
	printf("\n\n\n");
	printf("*****************************\n");
	printf("   WELCOME TO SIMPLE SHELL\n");
	printf("***************************** \n");
}

void	printDir()
{
	char	cwd[1024];
	char	*username = getenv("USER");

	getcwd(cwd, sizeof(cwd)); //get curret working directory
	printf("%s@%s$", username, cwd); //current username and directory
}

// to take input
int	take_input(char *str)
{
	char	*buf;

	buf = readline(">"); //display prompt and read input
	if (strlen(buf) != 0)
	{
		add_history(buf); //add input to command history 
		strcpy(str, buf); //copy input to the provided string
		return (0); // input received
	}
	free (buf);
	return (1); // no input
}

// splits the string by a delimiter and modifies the input str in place
char *ft_strsep(char **stringp, const char *delim)
{
	char	*start = *stringp;
	char	*current = start;

	if (*stringp == NULL)
		return (NULL);
	// Loop through the string until we find a delimiter
	 while (*current != '\0') 
	 {
		const char* delim_ptr = delim;
		// Check if current character matches any of the delimiter characters
		while (*delim_ptr != '\0') 
		{
			if (*current == *delim_ptr)
			{
				current = '\0';
				*stringp = current + 1;
				return (start);
			}
			delim_ptr++;
		}
		current++;
    }
	*stringp = NULL;  // No delimiter found, end of string
	return (start);
}

// returns number of pipes and splits the input based on all occurrences of the pipe character
int	parse_pipe(char *str, char **strpiped)
{
	int	i;

	i = 0;
	while ((strpiped[i] = ft_strsep(&str,  "|")) != NULL)
	{
		i++;
	}
	return (i - 1);
}

//splits the command string into individual tokens based on spaces.
void	parse_space(char *str, char **parsed)
{
	int	i;

	i = 0;
	while (i < MAXCOM)
	{
		parsed[i] = ft_strsep(&str, " ");
		if (parsed[i] == NULL)
			break ;
		if (strlen(parsed[i]) == 0) // ignore empty tokens
			continue ;
		i++;
	}
	parsed[i] = NULL; //NULL-TERMINATE THE PARSED ARRAY
}
// executing

void	exec_args(char **parsed)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
	{	
		printf("\nFailed forking child");
		return ;
	}
	else if (pid == 0)
	{
		if (execve(parsed[0], parsed, NULL) < 0)
			printf("\ncould not execute command");
		exit (0);
	}
	else
	{
		wait(NULL);
		return ;
	}
}

void	exec_args_piped(char **parsed, int num_pipes)
{
	int		pipefd[2 * num_pipes]; //pipe file descriptiors
	pid_t	pid;
	int		i;
	int		command_idx = 0;
	int		command;

	//create pipes
	i = 0;
	while (i < num_pipes)
	{
		if (pipe(pipefd + i * 2) < 0)
		{
			printf("\nPipe initialization failed");
			return ;
		}
		i++;
	}
	command = 0;
	i = 0;
	// looping through command in the pipeline
	while (command <= num_pipes)
	{
		pid = fork();
		if (pid == 0)
		{
			// If not the first command, read from the previous pipe
			if (command != 0)
				dup2(pipefd[(command - 1) * 2], STDIN_FILENO);
			// If not the last command, write to the next pipe
			if (command != num_pipes)
				dup2(pipefd[command * 2 + 1], STDOUT_FILENO);
			//close all pipe file descriptors
			while (i < 2 * num_pipes)
			{
				close(pipefd[i]);
				i++;
			}
			// // null terminate the arguments for execvp
			// int j = 0;
			// while (parsed[command_idx + j] != NULL)
			// 	j++;
			// parsed[command_idx + j] = NULL;
			// execute the command
			if (execve(parsed[command_idx], parsed, NULL) < 0)
			{
				printf("\ncould not execute command");
				exit (0);
			}
		}
		else if (pid < 0)
		{
			printf("\nfork failed");
			return ;
		}
		//move to the next command in the parsed array
		while (parsed[command_idx] != NULL)
			command_idx++;
		command_idx ++;
		command++;
	}
	i = 0;
	while (i < 2 * num_pipes)
	{
		close(pipefd[i]);
		i++;
	}
	i = 0;
	while (i <= num_pipes)
	{
		wait(NULL);
		i++;
	}
}

// handles built in commands like 'cd'
int	own_cmd_handler(char **parsed)
{
	int	i;
	if (strcmp(parsed[0], "cd") == 0)
	{	
		if (parsed[1] == NULL)
			printf("Expected argument to \"cd\"\n");
		else
			if (chdir(parsed[1]) != 0)
				perror("Shell");
		return (1);
	}
	if (strcmp(parsed[0], "pwd") == 0)
	{
		char cwd[1024];
		if (getcwd(cwd, sizeof(cwd)) != NULL)
			printf("%s\n",cwd);
		else
			perror("getcwd() error");
		return (1);
	}
	if (strcmp(parsed[0], "echo") == 0)
	{
		i = 1;
		while (parsed[i] != NULL)
		{
			printf("%s ", parsed[i]);
			i++;
		}
		return (1);
	}
	return (0);
}

//parsing input returns whether its a simple command or involes pipes
int	parse_input(char *str, char **parsed)
{
	char	*strpiped[MAXCOM];
	int		num_pipe;
	int		i = 0;

	num_pipe = parse_pipe(str, strpiped);
	if (num_pipe > 0)
	{
		//parsing each command
		while (i <= num_pipe)
		{
			parse_space(strpiped[i], parsed + (i * MAXCOM));
			i++;
		}
		exec_args_piped(parsed, num_pipe);
		return (0);
	}
	else
	{
		// handle simple commands
		parse_space(str, parsed);
		if (own_cmd_handler(parsed))
			return (0);
		else
			return (1);
	}
}

int	main(void)
{
	char	inputstring[MAXLT];
	char	*parsedArgs[MAXCOM];
	int		execflag;

	init_shell();
	while (1)
	{
		//pritning prompt line
		printDir();
		//take input
		take_input(inputstring);
		if (strcmp(inputstring, "exit") == 0)
			break ;
		execflag = parse_input(inputstring, parsedArgs);
		//execute
		if (execflag == 1)
			exec_args(parsedArgs);
	}
	return (0);
}

/*The paths passed to execve are assumed to be absolute. If the user provides a 
command like ls, it won't work unless /bin/ls or /usr/bin/ls is used.*/
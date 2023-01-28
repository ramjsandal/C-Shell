#include <stdio.h>
#include <assert.h>
#include "tokens.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
// prints the welcome message and then collects inputs
int main(int argc, char **argv) {

  printf("Welcome to mini-shell.\n");

  int run = takeInput(NULL);

  return 0;
}

// collects input from the user then
// tokenizes and passes it to other functions
int takeInput(char **prev) {

  // prints shell $ at the beginning of every line
  printf("shell $ ");

  // Creates a place to store the input
  char input[256];

  // Gets user input or ends if it runs into an EOF
  if (fgets(input, 256, stdin) == NULL) {
    printf("\nBye bye.\n");
    exit(0);
  }

  // Turns the user input into tokens
  char **tokens = get_tokens(input);

  // Make sure that the tokens are not null
  assert(tokens != NULL);

  char **current = tokens;

  // interpret the tokens
  int exit = splitAroundNullAndSemicolon(current, prev);

  if (exit == 1) {
    free_tokens(tokens);
    return 0;
  }

  // free memory we allocated and run the program again
  if (strcmp(current[0], "prev") == 0) {
    int runAgain = takeInput(prev);
    free_tokens(tokens);
    return 0;
  }  
  int runAgain = takeInput(tokens);
  free_tokens(tokens);

  return 0;

}
// checks if there are any semicolons in the input and executes around them
int splitAroundNullAndSemicolon(char **input, char **prev) {

  if (input[0] == NULL) {
    return 0;
  }

  char* *output = (char* *)malloc(256 * sizeof(char*));

  int o = 0;

  while (*input != NULL) {

    if (strcmp(*input, ";") == 0) {
      output[o] = NULL;
      if (checkPipes(output) == 1) {
        free_tokens(output);
	return 1;
      } else {
	o = 0;
	++input;
	free_tokens(output);
	return splitAroundNullAndSemicolon(input, prev);
      }
    
    } else {
      // then it must be a string or special char
      output[o] = (char *)malloc(strlen(*input) + 1);
      strcpy(output[o], *input);
    }
    ++o;
    ++input;
  }
  output[o] = NULL;

  if (checkPipes(output, prev) == 1) {
    free_tokens(output);
    return 1;
  }

  free_tokens(output);
  return 0;

}
// Checks if there are any pipes in the input
// and pipes the output of the thing that appears before into
// the next command
int checkPipes(char **input, char **prev) {

  char* *output = (char* *)malloc(256 * sizeof(char*));
  int o = 0;

  while (*input != NULL) {

    if (strcmp(*input, "|") == 0) {
    // dp nothing for now implement later

      // output is command no1
      output[o] = NULL;

      // recur on the rest of the input
      ++input;


      pid_t child_pid = fork();
      if (child_pid == 0) {
        // in child A
	int pipe_fds[2];

	assert(pipe(pipe_fds) == 0);

	int read_fd = pipe_fds[0];
	int write_fd = pipe_fds[1];

	pid_t child_pid2 = fork();

	if (child_pid2 == 0) {

	  // in child B
	  close(read_fd);

          if (close(1) == -1) {
            perror("Error closing stdout");
	    exit(1);
	  }
	  // hook pipe to stdout
	  assert(dup(write_fd) == 1);

	  // execute command no1
	  if (execvp(output[0], output) == -1) {
            perror("Error - execvp failed");
	    exit(1);
	  }
	  exit(0);
	} else {
          // back in child A
	  int status2;
	  pid_t exited_child2 = waitpid(child_pid2, &status2, 0);
	  close(write_fd);

	  if (close(0) == -1) {
            perror("Error closing stdin");
	    exit(1);
	  }

	  assert(dup(read_fd) == 0);
           
	  int val = checkPipes(input, prev);
	  exit(0);

	}
        

      } else {
	// in parent
	int status;
	pid_t exited_child = waitpid(child_pid, &status, 0);
	free_tokens(output);
	return 0;
      }

    } else {
      output[o] = (char *)malloc(strlen(*input) + 1);
      strcpy(output[o], *input);
    }
    ++o;
    ++input;
  }

  output[o] = NULL;

  if (parseFunction(output, prev) == 1) {
    free_tokens(output);
    return 1;
  }

  free_tokens(output);
  return 0;

}

// executes the input, which can now only contain
// strings, <, >, and builtins
int parseFunction(char **input, char **prev) {

  char* *output = (char* *)malloc(256 * sizeof(char*));
  int o = 0;

  while (*input != NULL) {

    if (strcmp(*input, "<") == 0) {
      // need to redirect input
      ++input;
      char* file = *input;
      
      ++input;
      while (*input != NULL) {
        output[o] = (char *)malloc(strlen(*input) + 1);
	strcpy(output[o], *input);
	++input;
	++o;
      }
      output[o] = NULL;

      pid_t child2 = fork();

      if (child2 == -1) {
        perror("Error - fork failed");
	exit(1);
      }
      if (child2 == 0) {
        if (close(0) == -1) {
          perror("Error closing stdin");
	  exit(1);
	}

	int fd = open(file, O_RDONLY);
	assert(fd == 0);
	
	int value = parseFunction(output, prev);
	exit(0);
      }

      int status2;
      pid_t exited_child2 = waitpid(child2, &status2, 0);

      free_tokens(output);
      return 0;


    } else if (strcmp(*input, ">") == 0) {
      // need to redirect output
      ++input;
      char* outFile = *input;

      ++input;

      while (*input != NULL) {
        output[o] = (char *)malloc(strlen(*input) + 1);
	++input;
	++o;
      }

      output[o] = NULL;

      pid_t child = fork();

      if (child == -1) {
        perror("Error - fork failed");
	exit(1);
      }

      if (child == 0) {
        if (close(1) == -1) {
	  perror("Error closing stdout");
	  exit(1);
	}

	int fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	assert(fd == 1);

	int value = parseFunction(output, prev);
	exit(0);

      }

      //back in parent
      int status;
      pid_t exited_child = waitpid(child, &status, 0);
      
      free_tokens(output);
      return 0;

    } else {
      output[o] = (char *)malloc(strlen(*input) + 1);
      strcpy(output[o], *input);
    }
    ++o;
    ++input;
  }
  output[o] = NULL;

  if (executor(output[0], output, prev) == 1) {
    free_tokens(output);
    return 1;
  }

  free_tokens(output);
  return 0;
}
// executes commands, including builtins
int executor(const char* initCommand, char* const command[], char **previous) {
  // status of child
  int cmdStatus;
  // checks if the user wants to exit
  if (strcmp(initCommand, "exit") == 0) {
    printf("Bye bye.\n");
    return 1;
    // checks if the user typed cd
  } else if (strcmp(initCommand, "cd") == 0) {
    changeDirectory(command[1]);
    // checks if the user typed help
  } else if (strcmp(initCommand, "help") == 0) {
    printf("cd changes the current working directory to the indicated directory\nsource takes in a filename and executes the file as a script\nprev executes the previous command line and prints it\nhelp explains all of the built in commands\nexit exits the program\n");
    // checks if the user typed course
  } else if (strcmp(initCommand, "source") == 0) {
    char buffer[256];

    FILE *f = fopen(command[1], "r");

    if (f == NULL) {
      perror("Error trying to open file");
      exit(1);
    }

    if (fgets(buffer, 256, f) == NULL) {
      printf("\nBye bye.\n");
      exit(0);
    }

    fclose(f);
    

    char** tokens = get_tokens(buffer);

    assert(tokens != NULL);

    char **current = tokens;

    int end = splitAroundNullAndSemicolon(current, previous);

    if (end == 1) {
      free_tokens(tokens);
      return 0;
    }
    free_tokens(tokens);
    return 0;
    // checks if the user typed prev
  } else if (strcmp(initCommand, "prev") == 0) {
    // if they enter prev with no previous commands, just ignore it
    if (previous == NULL) {
      return 0;	    
    } 

    // print out the string
    int k = 0;
    while (previous[k] != NULL) {
      printf("%s ", previous[k]);
      k++;
    }
    printf("\n");
    // run the command again
    return splitAroundNullAndSemicolon(previous, previous);

  } else {
  // executes any other command
  if (fork() == 0) {
    execvp(initCommand, command);
    exit(1);
  } else {
    wait(&cmdStatus);
    if (WEXITSTATUS(cmdStatus) != 0) {
      printf("%s: command not found\n", initCommand);
    }
  }

  }

  return 0;

}

int changeDirectory(const char* directory) {

  if (directory == NULL) {
    chdir("..");
    return 0;
  } else if (chdir(directory) != 0) {
    printf("Failed to change directories\n");
  }
  return 0;

}












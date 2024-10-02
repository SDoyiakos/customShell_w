#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include "wsh.h"

extern char** environ;

/**
*	Test function merely for me to see if tokens are being grabbed properly
**/
void debugTokens(TokenArr my_tokens) {
	printf("The tokens in the arr are:\n");
	for(int i = 0;i < my_tokens.token_count;i++) {
		printf("%s\n", my_tokens.tokens[i]);
	}
}

TokenArr tokenizeString(char* my_str, int input_size) {
	TokenArr my_tokens;
	my_tokens.token_count = 0;
	const char DELIM = ' ';
	
	// Allocated char count + 1 as we need the /0
	char* my_str_cpy = malloc(sizeof(char) + (input_size * sizeof(char)));
	if(my_str_cpy == NULL) {
		exit(-1);
	}

	// Copy string
	if(strcpy(my_str_cpy, my_str) == NULL) {
		exit(-1);
	}
	
	// Get amount of tokens
	if(strtok(my_str_cpy, &DELIM) != NULL) {
		my_tokens.token_count++;
	}
	while(strtok(NULL, &DELIM) != NULL) {
		my_tokens.token_count++;
	}

	// Allocate token arr
	my_tokens.tokens = malloc(my_tokens.token_count * sizeof(char*));
	if(my_tokens.tokens == NULL) {
		printf("Error retrieving command\nExiting\n");
		exit(-1);
	}

	// Copy tokens into token arr
	
	if(my_tokens.token_count > 0) {
		char* token_buf;
		token_buf = strtok(my_str, &DELIM);
		my_tokens.tokens[0] = malloc(strlen(token_buf) + sizeof(char));
		strcpy(my_tokens.tokens[0], token_buf);
	
		for(int i = 1; i < my_tokens.token_count;i++) {
			token_buf = strtok(NULL, &DELIM); // Check for errors later
			my_tokens.tokens[i] = malloc(strlen(token_buf) + sizeof(char));

			if(my_tokens.tokens[i] == NULL) {
				printf("Error Parsing Input\n");
				exit(-1);
			}
			
			strcpy(my_tokens.tokens[i], token_buf);
		}
	}
	free(my_str_cpy); // Free the copy
	return my_tokens;	
}

void parseInputs(char* input_buffer, int* input_size) {
	
	size_t buffer_len = SHELL_MAX_INPUT; // The size of the buffer
	
	// Getting next user input
	*input_size = getline(&input_buffer, &buffer_len, stdin);

	// Checking whether input has errors or not
	if(*input_size == -1) {
		// The two errno flags that can be set on a bad read
		if(errno == EINVAL || errno == ENOMEM) {
			printf("Error reading new line\nExiting\n");
			exit(-1);
		}
	}

	// Sanitize string to not include \n
	else if(*input_size > 1){
		input_buffer[*input_size - 1] = '\0';
		(*input_size)--;
	}
}


void interactiveMode() {
	TokenArr my_tokens;
	char* user_input = malloc(SHELL_MAX_INPUT * sizeof(char));
	int* input_size = malloc(sizeof(int));

	// Run loop until exit
	while(1) {
		printf("wsh> ");
		parseInputs(user_input, input_size);	

		// Check for EOF after getting input
		if(feof(stdin)) {
			wshExit();
		}
		my_tokens = tokenizeString(user_input, *input_size); // Tokenize input
		runCommand(my_tokens);


		for(int i =0; i < my_tokens.token_count;i++) {
			free(my_tokens.tokens[i]);
		}
		free(my_tokens.tokens);
	}

	exit(0);
	
}

/**
* Shell built in exit function
* Calls exit(0) syscall
**/
void wshExit() {
	exit(0);
}

/**
* Shell built in ls function
* Performs like bash's ls -1
**/
void wshLs() {
	DIR* dp;
	struct dirent* my_dirent;

	// Open dir and check that the open worked
	dp = opendir(".");
	if(dp == NULL) {
		printf("Could not run ls on current directory\n");
		exit(-1);
	}

	// Read until null
	my_dirent = readdir(dp);
	while(my_dirent != NULL) {
		if((my_dirent->d_name)[0] != '.') {
			printf("%s\n", my_dirent->d_name);
		}	
		my_dirent = readdir(dp);
	}

	// Check for errors
	if(errno == EBADF) {
		printf("Could not run ls on current directory\n");
		exit(-1);
	}

	closedir(dp);
}

/**
*	Shell change directory command.
*	Takes param to new dir 
**/
void wshCd(char* new_dir){
	if(chdir(new_dir) != 0) {
		printf("Error, could not cd to directory %s\n", new_dir);
		exit(-1);
	}
}

void wshExport(char* var_name, char* var_val) {
	int ret_val;
	ret_val = setenv(var_name, var_val, 1); // Change and overwriting

	if(ret_val == -1) {
		printf("Error setting environment variable\n");
		exit(-1);
	}
	
}

void wshVars() {
	char* env_val;
	int env_index = 0;
	env_val = environ[env_index];
	while(env_val != NULL) {
		printf("%s\n", env_val);
		env_index++;
		env_val = environ[env_index];
	}
}

/**
* Gets the index of a built in command in the COMMAND_ARR
* Returns -1 if command isnt built in
**/
int checkBuiltIn(char* my_command) {
	int commands_size = (sizeof(COMMAND_ARR)/sizeof(char*));
	for(int i = 0; i < commands_size;i++) {
		if(strcmp(my_command, COMMAND_ARR[i]) == 0) {
			return i;
		}
	}
	return -1;
}

void runCommand(TokenArr my_tokens) {
	char* my_command = my_tokens.tokens[0];

	switch(checkBuiltIn(my_command)) {
		case 0: // exit

			// Checking for zero flags or parameters
			if(my_tokens.token_count > 1) {
				printf("Error, exit should be used with no parameters\n");
			}
			else {
				wshExit();
			}
			break;
			
		case 1: // ls
		
			// Checking for zero flags or parameters
			if(my_tokens.token_count > 1) {
				printf("Error, ls should be used with no parameters\n");
			}
			else{
				wshLs();
			}
			break;
		case 2: // cd

			// Checking for exactly one arg
			if(my_tokens.token_count != 2) {
				printf("Error, cd should be used with a single argument\n");
			}
			else {
				wshCd(my_tokens.tokens[1]);
			}
			break;
		case 3:

			if(my_tokens.token_count != 2) {
				printf("Error, export should be used in the manner, export VAR=value\n");
			}
			else {
				char* var_name;
				char* var_val;
				var_name = strtok(my_tokens.tokens[1], "=");
				var_val = strtok(NULL, "=");

				// If var_name or var_val is null
				if(var_name == NULL || var_val == NULL) {
					printf("Error, export failed to assign environ var\n");
				}
				else {
					wshExport(var_name, var_val);
				}
			}
			break;
		case 5: // vars
			wshVars();
			break;	
	}
}

int main(int argc, char* argv[]) {
	if(argc == 1) {
		interactiveMode();
	}
}

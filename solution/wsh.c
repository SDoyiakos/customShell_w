#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "wsh.h"

// Global vars
extern char** environ;
struct ShellVar* shellLinkedListHead = NULL;
struct ShellVar* shellLinkedListTail = NULL;

// History globals
struct HistEntry* histHead;
struct HistEntry* histTail;
int histLimit = 5;
int histSize = 0;

/**
* Separates the input across token '='.
* Returns a TokenArr of the inputs
**/
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
	my_tokens.tokens = malloc( (my_tokens.token_count * sizeof(char*)) + sizeof(char*) );
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
	my_tokens.tokens[my_tokens.token_count] = NULL; // Last entry null for execve if needed
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
	else if(*input_size >= 1){
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

		if(*input_size != 0) {
			my_tokens = tokenizeString(user_input, *input_size); // Tokenize input
			runCommand(my_tokens, user_input);
			for(int i =0; i < my_tokens.token_count;i++) {
				free(my_tokens.tokens[i]);
			}
			free(my_tokens.tokens);
		}
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
int wshLs() {
	int dir_ret;
	struct dirent** my_dirent;
	// Read until null
	dir_ret = scandir(".", &my_dirent, 0, alphasort);
	if(dir_ret == -1) { // Error case
		return -1;
	}
	for(int i = 0;i < dir_ret; i++) {
		if(my_dirent[i]->d_name[0] != '.') {
			printf("%s", my_dirent[i]->d_name);
			if(my_dirent[i]->d_type == DT_DIR) {
				printf("/");
			}
			printf("\n");
		}
	}
	return 0;
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

/**
* Sets environment variable var_name=var_val
**/
void wshExport(char* var_name, char* var_val) {
	int ret_val;
	ret_val = setenv(var_name, var_val, 1); // Change and overwriting

	if(ret_val == -1) {
		printf("Error setting environment variable\n");
		exit(-1);
	}
	
}

/**
* Prints all shell vars
**/
void wshVars() {
	struct ShellVar* var_ptr;
	var_ptr = shellLinkedListHead;

	// Read until var is null
	while(var_ptr != NULL) {
		printf("%s=%s\n",var_ptr->var_name, var_ptr->var_val);
		var_ptr = var_ptr->next_var;
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

/**
* Gets the index of a shell variable
**/
int findShellVar(char* var_name) {
	int index = 0;
	struct ShellVar* var_ptr = shellLinkedListHead;

	// Linear search for var index
	while(var_ptr != NULL) {

		// Var found
		if(strcmp(var_ptr->var_name,var_name) == 0){
			return index;
		}
		var_ptr = var_ptr->next_var;
		index++;
	}
	return -1; // Var not found
}

/**
* Built in wsh command. Adds the variable to the
* local vars array. Updates value instead if already found in local vars array
**/
void wshLocal(char* var_name, char* var_val) {
	struct ShellVar* shell_var_ptr;
	int var_loc;
	var_loc = findShellVar(var_name);
	
	// If the linked list is empty/has no entries
	if(shellLinkedListHead == NULL) {

		// Allocating head and tail
		shellLinkedListHead = malloc(sizeof(struct ShellVar));
		if(shellLinkedListHead == NULL) {
			printf("Error adding shell var\n");
			exit(-1);
		}
		shellLinkedListTail = shellLinkedListHead;
		shellLinkedListHead->next_var = NULL;
		shell_var_ptr = shellLinkedListHead; // New ptr is to head
	}

	// If var is in the Linked List
	else if(var_loc != -1) {
		shell_var_ptr = shellLinkedListHead;
		for(int i = 0;i < var_loc;i++) {
			shell_var_ptr = shell_var_ptr->next_var;
		}
	}
	// If there is a head to the Linked List
	else {
	
		shell_var_ptr = shellLinkedListTail;
		
		// Allocate next ShellVar in linked list
		shell_var_ptr->next_var = malloc(sizeof(struct ShellVar));
		if(shell_var_ptr == NULL) {
			printf("Error adding shell var\n");
			exit(-1);
		}

		shell_var_ptr = shell_var_ptr->next_var; // Point to new var
		shell_var_ptr->next_var = NULL; // Set new vars next var to null
		shellLinkedListTail = shell_var_ptr; // Set new tail
	}
	
	// Copy vals into new var
	if(strcpy(shell_var_ptr->var_name, var_name) == NULL || strcpy(shell_var_ptr->var_val, var_val) == NULL) {
		printf("Error adding shell var\n");
		exit(-1);
	}	
}

void addHistEntry(char* command) { 
	char error_message[] = "Error adding command to history";

	// Adding first entry
	if(histSize == 0) {
		histHead = malloc(sizeof(struct HistEntry));
		histTail = histHead;
		if(histHead == NULL || histTail == NULL) {
			printf("%s\n", error_message);
			exit(-1);
		}
		if(strcpy(histHead->command, command) == NULL) {
			printf("%s\n", error_message);
			exit(-1);
		}
		histHead->next_entry = NULL;
		histHead->prev_entry = NULL;
		histSize++;
	}

	// >= 1 Entry
	else if(strcmp(histHead->command, command) != 0){
		struct HistEntry* command_ptr;
		command_ptr = malloc(sizeof(struct HistEntry));
		if(command_ptr == NULL) {
			printf("%s\n", error_message);
			exit(-1);
		}
		if(strcpy(command_ptr->command, command) == NULL) {
			printf("%s\n", error_message);
			exit(-1);
		}
		command_ptr->next_entry = histHead;
		histHead->prev_entry = command_ptr;
		command_ptr->prev_entry = NULL;
		histHead = command_ptr;
		histSize++;
	}

	// Remove any entries over limit 
	while(histSize > histLimit) {
		removeHistEntry();
	}
}

void wshGetHist() {
	struct HistEntry* hist_ptr = histHead;
	for(int i = 0; i < histSize; i++) {
		printf("%d) %s\n", (i+1), hist_ptr->command);
		hist_ptr = hist_ptr->next_entry;
	}
}

void wshSetHist(int new_limit) {
	int size_diff;
	histLimit = new_limit;

	size_diff = histSize - histLimit;

	while(size_diff > 0) {
		removeHistEntry();
		size_diff = histSize - histLimit;
	}	
}

void removeHistEntry() {
	struct HistEntry* hist_ptr = histTail;
	histTail = hist_ptr->prev_entry;
	free(histTail->next_entry);
	histTail->next_entry = NULL;
	histSize--;
}

/**
* Retrieves the path to a program, first full or relative and then $PATH
**/
char* getPath(TokenArr my_tokens) {
	int acc_val;

	// Check if in wd
	acc_val = access(my_tokens.tokens[0], X_OK);
	if(acc_val == 0) {
		return my_tokens.tokens[0];
	}
	else {
		char* path_ptr;
		char* path_str;
		char* full_dir_ptr;

		path_str = malloc(sizeof(char) * MAX_DIR_SIZE);
		if(path_str == NULL) {
			printf("Error getting path\n");
			exit(-1);
		}
		if(strcpy(path_str, getenv("PATH")) == NULL) {
			printf("Error getting path\n");
			exit(-1);
		}

		// Allocate mem for the dir
		full_dir_ptr = malloc(sizeof(char) * MAX_DIR_SIZE);
		if(full_dir_ptr == NULL) {
			printf("Error getting path\n");
			exit(-1);
		}

		// Check for each path
		path_ptr = strtok(path_str, ":");
		while(path_ptr != NULL) {

			full_dir_ptr[0] = '\0'; // Clearing str
			
			// Concat for full path
			strcat(full_dir_ptr, path_ptr);
			strcat(full_dir_ptr, "/");
			strcat(full_dir_ptr, my_tokens.tokens[0]);

			// Check for exec access
			if(access(full_dir_ptr, X_OK) == 0) {
				return full_dir_ptr;
			}

			free(path_ptr); 
			path_ptr = strtok(NULL, ":"); // Get new path
			
		}
		return NULL; // No path found
	}
	
}

int runCommand(TokenArr my_tokens, char* user_input) {
	char* my_command = my_tokens.tokens[0];
	char* path_val;
	int fork_val;
	switch(checkBuiltIn(my_command)) {

		case -1: // Non built in command
			
			path_val = getPath(my_tokens);
			if(path_val == NULL) {
				printf("Not a valid command\n");
				return -1;
			}

			fork_val = fork();

			if(fork_val > 0) { // Parent
				wait(NULL);
				addHistEntry(user_input);
				free(path_val);
			}
			else if(fork_val == 0) { // Child
				execve(path_val, my_tokens.tokens, environ);
			}
			else { // ERROR
				printf("Error executing in child\n");
				return -1;
			}
			break;
			
		case 0: // exit

			// Checking for zero flags or parameters
			if(my_tokens.token_count > 1) {
				printf("Error, exit should be used with no parameters\n");
				return -1;
			}
			else {
				wshExit();
			}
			break;
			
		case 1: // ls
		
			// Checking for zero flags or parameters
			if(my_tokens.token_count > 1) {
				printf("Error, ls should be used with no parameters\n");
				return -1;
			}
			else{
				wshLs();
			}
			break;
		case 2: // cd

			// Checking for exactly one arg
			if(my_tokens.token_count != 2) {
				printf("Error, cd should be used with a single argument\n");
				return -1;
			}
			else {
				wshCd(my_tokens.tokens[1]);
			}
			break;
		case 3:

			if(my_tokens.token_count != 2) {
				printf("Error, export should be used in the manner, export VAR=value\n");
				return -1;
			}
			else {
				char* var_name;
				char* var_val;
				var_name = strtok(my_tokens.tokens[1], "=");
				var_val = strtok(NULL, "=");

				// If var_name or var_val is null
				if(var_name == NULL || var_val == NULL) {
					printf("Error, export failed to assign environ var\n");
					return -1;
				}
				else {
					wshExport(var_name, var_val);
				}
			}
			break;

		case 4:
			printf("Running Local\n");
			if(my_tokens.token_count !=2) {
				printf("Invalid input for local\n");
				return -1;
			}
			else {
			
				char* var_name;
				char* var_val;
				var_name = strtok(my_tokens.tokens[1], "=");
				var_val = strtok(NULL, "=");
				
				// If var_name or var_val is null
				if(var_name == NULL || var_val == NULL) {
						printf("Error, export failed to assign shell var\n");
						return -1;
				}
				else {
					wshLocal(var_name, var_val);
				}
			}
			break;
		
		case 5: // vars
			wshVars();
			break;	

		case 6: // history
			if(my_tokens.token_count == 1) {
				wshGetHist();
			}
			else if(my_tokens.token_count == 3) {
				if(strcmp(my_tokens.tokens[1],"set") !=0) {
					printf("Invalid input for history\n");
				}
					else {
					int my_val;
					my_val = atoi(my_tokens.tokens[2]);
					if(my_val <= 0 ) {
						printf("Invalid input for history\n");
						return -1;
					}
					else {
						wshSetHist(my_val);
					}
				}
			}
	}
	return -1;
}

int main(int argc, char* argv[]) {
	wshExport("PATH", "/bin");
	if(argc == 1) {
		interactiveMode();
	}
}

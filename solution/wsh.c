#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "wsh.h"

// Global vars
extern char** environ;
struct ShellVar* shellLinkedListHead = NULL;
struct ShellVar* shellLinkedListTail = NULL;
int exit_global = 0;

// History globals
struct HistEntry* histHead;
struct HistEntry* histTail;
int histLimit = 5;
int histSize = 0;

// Globals to restore redirects
int original_desc = -1;
int new_desc = -1;
int second_original_desc = -1;
int second_new_desc = -1;

TokenArr* tokenizeString(char* my_str) {
	char error_message[] = "Error tokenizing string";
	char* next_token;
	int token_arr_size;
	TokenArr* my_tokens = malloc(sizeof(TokenArr));
	if(my_tokens == NULL) {
		printf("%s\n", error_message);
	}
	
	my_tokens->token_count = 0;
	token_arr_size = 10; // Start with array of limit
	my_tokens->tokens = malloc(token_arr_size * sizeof(char*));
	if(my_tokens->tokens == NULL) {
		printf("%s\n", error_message);
		return NULL;
	}
	
	next_token = strtok(my_str, " ");
	while(next_token != NULL) {
	
		// Check if there is enough space and realloc otherwise
		if(my_tokens->token_count + 2 == token_arr_size) {
			char* alloc_ret = NULL;
			alloc_ret = realloc(my_tokens->tokens, (token_arr_size * sizeof(char*)) + sizeof(char*));
			if(alloc_ret == NULL) {
				fprintf(stderr, "%s\n", error_message);
				return NULL;
			}
			token_arr_size++;
		}
		my_tokens->tokens[my_tokens->token_count] = strdup(next_token);
		my_tokens->token_count++;
		next_token = strtok(NULL, " ");
		
	}

	my_tokens->tokens[my_tokens->token_count] = NULL; // Use for terminating as args
	return my_tokens;	
}

int parseInputs(char* input_buffer, int* input_size, FILE* input_stream) {	
	size_t buffer_len = SHELL_MAX_INPUT; // The size of the buffer
	
	// Getting next user input
	*input_size = getline(&input_buffer, &buffer_len, input_stream);

	// Checking whether input has errors or not
	if(*input_size == -1) {
		// The two errno flags that can be set on a bad read
		if(errno == EINVAL || errno == ENOMEM) {
			fprintf(stderr, "Error reading new line\nExiting\n");
			return -1;
		}
	}

	// Sanitize string to not include \n
	else if(*input_size >= 1){
		input_buffer[*input_size - 1] = '\0';
		(*input_size)--;
	}
	return 0;
}

int substituteShellVars(TokenArr* my_tokens) {
	char* my_var;
	char* shortened_input;
	for(int i = 0;i< my_tokens->token_count;i++) {
		if(my_tokens->tokens[i][0] == '$') { // If token is a var
			shortened_input = &my_tokens->tokens[i][1]; // Retrieve str without the $
			
			// Get from env first
			my_var = getenv(shortened_input);

			// If var not found in env
			if(my_var == NULL) {
				my_var = getShellVar(shortened_input); // Get the vars value
			}
			// Replacing the token with the var's value
			free(my_tokens->tokens[i]);
			my_tokens->tokens[i] = malloc(strlen(my_var) + 1);
			if(my_tokens->tokens[i] == NULL) {
				fprintf(stderr, "Malloc error\n");
				return -1;
			}
			if(strcpy(my_tokens->tokens[i], my_var) == NULL) {
				fprintf(stderr, "Substitute copy error\n");
				return -1;
			}
		}
	}
	return 0;
}

void programLoop(FILE* input_stream) {
	TokenArr* my_tokens;
	char user_input[SHELL_MAX_INPUT];
	int input_size;
	char* redirect_val = NULL;

	// Run loop until exit
	while(1) {
		if(input_stream == stdin) {
			printf("wsh> ");
			fflush(stdout);
		}	

		// Don't want to execute further if cant parse
		if(parseInputs(user_input, &input_size, input_stream) == -1) {
			exit_global = -1;
			continue;
		}
		
		// Check for EOF after getting input
		if(feof(input_stream)) {
			wshExit();
		}

		if(input_size != 0) {
			my_tokens = tokenizeString(user_input); // Tokenize input
			if(my_tokens->tokens[0][0] != '#') {				
				if(substituteShellVars(my_tokens) == -1) {
					exit_global = -1;
					freeTokenArr(my_tokens);
					continue;
				}
				
				// Check for redirect
				redirect_val = getRedirect(my_tokens->tokens[my_tokens->token_count - 1]);
				if(redirect_val != NULL) {

					// Last token not part of command
					if(performRedirect(redirect_val, my_tokens->tokens[my_tokens->token_count - 1]) == -1) {
						exit_global = -1;
						freeTokenArr(my_tokens);
						continue;
					}
					free(my_tokens->tokens[my_tokens->token_count -1]);
					my_tokens->tokens[my_tokens->token_count -1] = NULL;
					my_tokens->token_count--;
				}
				exit_global = runCommand(my_tokens);
				restoreFileDescs();
			}
			freeTokenArr(my_tokens);
		}
	}
	wshExit();
}

int checkBuiltIn(char* my_command) {
	int commands_size = (sizeof(COMMAND_ARR)/sizeof(char*));
	for(int i = 0; i < commands_size;i++) {
		if(strcmp(my_command, COMMAND_ARR[i]) == 0) {
			return i;
		}
	}
	return -1;
}

char* getShellVar(char* var_name) {
	struct ShellVar* my_var;
	my_var = shellLinkedListHead;
	while(my_var != NULL) {
		if(strcmp(my_var->var_name, var_name) == 0) {
			return my_var->var_val;
		}
		my_var = my_var->next_var;
	}
	return "";
}

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

int tokenCmp(TokenArr* arr1, TokenArr* arr2) {

	// Compare they're same size
	if(arr1->token_count != arr2->token_count) {
		return 0;
	}

	// Compare each token
	for(int i = 0; i < arr1->token_count;i++) {
		if(strcmp(arr1->tokens[i],arr2->tokens[i]) != 0) {
			return 0;
		}
	}
	return -1;
}

TokenArr* copyTokenArr(TokenArr* my_tokens) {
	char error_message[] = "Error copying token arr";
	TokenArr* my_copy;

	// Allocating copy
	my_copy = malloc(sizeof(TokenArr));
	if(my_copy == NULL) {
		printf("%s\n", error_message);
		return NULL;
	}
	my_copy->token_count = my_tokens->token_count;

	// Allocating copy's tokens arr
	my_copy->tokens = malloc((my_copy->token_count + 1) * sizeof(char*));
	if(my_copy->tokens == NULL) {
		printf("%s\n", error_message);
		return NULL;
	}

	// Copy values over
	for(int i = 0;i < my_copy->token_count;i++) {
		my_copy->tokens[i] = malloc(strlen(my_tokens->tokens[i]) + sizeof(char*));
		if(my_copy->tokens[i] == NULL) {
			printf("%s\n", error_message);
			return NULL;
		}
		if(strcpy(my_copy->tokens[i], my_tokens->tokens[i]) == NULL)  {
			printf("%s\n", error_message);
			return NULL;
		}
	}
	my_copy->tokens[my_copy->token_count] = NULL; // Terminating null when used as args
	return my_copy;
}

int addHistEntry(TokenArr* my_tokens) { 
	char error_message[] = "Error adding command to history";

	// Adding first entry
	if(histSize == 0) {
		histHead = malloc(sizeof(struct HistEntry));
		histTail = histHead;
		if(histHead == NULL || histTail == NULL) {
			fprintf(stderr, "%s\n", error_message);
			return -1;
		}

		// Create a copy of the tokens at head entry
		histHead->entry_tokens = copyTokenArr(my_tokens);
		histHead->next_entry = NULL;
		histHead->prev_entry = NULL;
		histSize++;
	}

	// >= 1 Entry
	else if(tokenCmp(histHead->entry_tokens, my_tokens) != 1){
		struct HistEntry* command_ptr;
		command_ptr = malloc(sizeof(struct HistEntry));
		if(command_ptr == NULL) {
			fprintf(stderr, "%s\n", error_message);
			return -1;
		}
		command_ptr->entry_tokens = copyTokenArr(my_tokens);
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
	return 0;
}

struct HistEntry* getHistEntry(int index) {
	index = index-1;
	struct HistEntry* ret_val;
	ret_val = histHead;
	for(int i = 0; i < index;i++) {
		ret_val = ret_val->next_entry;
	}
	return ret_val;
}

void freeTokenArr(TokenArr* my_tokens) {
	for(int i = 0;i < my_tokens->token_count;i++) {
		free(my_tokens->tokens[i]);
	}
	free(my_tokens->tokens);
	free(my_tokens);
}

void removeHistEntry() {
	struct HistEntry* hist_ptr = histTail;
	freeTokenArr(hist_ptr->entry_tokens);
	histTail = hist_ptr->prev_entry;
	free(histTail->next_entry);
	histTail->next_entry = NULL;
	histSize--;
}

void freeHistory() {
	struct HistEntry* next_entry_ptr;
	struct HistEntry* current_entry_ptr;
	current_entry_ptr = histHead;
	while(current_entry_ptr != NULL) {
		next_entry_ptr = current_entry_ptr->next_entry;
		freeTokenArr(current_entry_ptr->entry_tokens);
		free(current_entry_ptr);
		current_entry_ptr = next_entry_ptr;
	}
}

void freeShellVars() {
	struct ShellVar* next_entry_ptr;
	struct ShellVar* current_entry_ptr;
	current_entry_ptr = shellLinkedListHead;
	while(current_entry_ptr != NULL) {
		next_entry_ptr = current_entry_ptr->next_var;
		free(current_entry_ptr);
		current_entry_ptr = next_entry_ptr;
	}
}

char* getPath(TokenArr* my_tokens) {
	char error_message[] = "Error getting path";
	int acc_val;
	char* command_cpy;
	command_cpy = malloc(strlen(my_tokens->tokens[0]) + sizeof(char*));
	if(command_cpy == NULL) {
		return NULL;
	}

	if(strcpy(command_cpy, my_tokens->tokens[0]) == NULL) {
		fprintf(stderr, "%s\n", error_message);
		return NULL;
	}
	
	// Check if in wd
	acc_val = access(command_cpy, X_OK);
	if(acc_val == 0) {
		return command_cpy;
	}
	else {
		char* path_ptr;
		char* path_str;
		char* full_dir_ptr;

		path_str = malloc(sizeof(char) * MAX_DIR_SIZE);
		if(path_str == NULL) {
			fprintf(stderr, "%s\n", error_message);
			free(command_cpy);
			return NULL;
		}

		// Copy path var into path_str
		if(strcpy(path_str, getenv("PATH")) == NULL) {
			fprintf(stderr, "%s\n", error_message);
			free(command_cpy);
			return NULL;
		}

		// Allocate mem for the dir
		full_dir_ptr = malloc(sizeof(char) * MAX_DIR_SIZE);
		if(full_dir_ptr == NULL) {
			fprintf(stderr, "%s\n", error_message);
			free(command_cpy);
			return NULL;
		}

		// Check for each path
		path_ptr = strtok(path_str, ":");
		while(path_ptr != NULL) {

			full_dir_ptr[0] = '\0'; // Clearing str
			
			// Concat for full path
			strcat(full_dir_ptr, path_ptr);
			strcat(full_dir_ptr, "/");
			strcat(full_dir_ptr, command_cpy);
			
			// Check for exec access
			if(access(full_dir_ptr, X_OK) == 0) {
				free(path_str);
				free(command_cpy);
				return full_dir_ptr;
			}
			path_ptr = strtok(NULL, ":"); // Get new path
			strcpy(command_cpy, my_tokens->tokens[0]);
			
		}
		free(full_dir_ptr);
		free(path_str);
		free(command_cpy);
		return NULL; // No path found
	}
	
}

char* getRedirect(char* my_token) {
	if(strstr(my_token, "&>>") != NULL) {
		return "&>>";
	}
	else if (strstr(my_token, "&>") != NULL) {
		return "&>";
	}
	else if(strstr(my_token, ">>") != NULL) {
		return ">>";
	}
	else if(strstr(my_token, ">") != NULL) {
		return ">";
	}
	else if(strstr(my_token, "<") != NULL) {
		return "<";
	}
	else {
		return NULL;
	}
}

int outputAppend(char* lhs, char* rhs) {
	int lhs_file = atoi(lhs);
	int rhs_file = open(rhs, O_WRONLY | O_CREAT | O_APPEND, 
	S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP );

	// Error checking
	if(lhs_file < 0 || rhs_file == -1) {
		return -1;
	}

	// Redirect lhs to rhs
	original_desc = lhs_file;
	new_desc = dup(original_desc);
	return dup2(rhs_file, lhs_file);	
}

int inputRedirect(char* lhs, char* rhs) {
	int lhs_file = atoi(lhs);
	int rhs_file = open(rhs, O_RDONLY);	

	// Error checking
	if(rhs_file == -1 || lhs_file < 0) {
		return -1;
	}

	// Redirect rhs as input to lhs
	original_desc = lhs_file;
	new_desc = dup(original_desc);
	return dup2(rhs_file, lhs_file);
}

int outputRedirect(char* lhs, char* rhs) {
	int lhs_file = atoi(lhs);
	int rhs_file = open(rhs, O_WRONLY | O_TRUNC | O_CREAT,
	 S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP );

	// Error checking
	if(lhs_file < 0 || rhs_file == -1) {
		return -1;
	}

	// Redirect lhs to rhs
	original_desc = lhs_file;
	new_desc = dup(original_desc);
	return dup2(rhs_file, lhs_file);
}

int outputErrRedirect(char* rhs) {
	int ret_val;
	int rhs_file = open(rhs, O_WRONLY| O_TRUNC | O_CREAT,
		 S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP );

	if(rhs_file == -1) {
		return -1;
	}

	// Redirect stdout to rhs
	original_desc = 1;
	new_desc = dup(original_desc);
	ret_val = dup2(rhs_file, 1);
	if(ret_val == -1) {
		return -1;
	}

	// Redirect stderr to rhs
	second_original_desc = 2;
	second_new_desc = dup(second_original_desc);
	ret_val = dup2(rhs_file, 2);
	if(ret_val == -1) {
		return -1;
	}
	return 0;	 
}

int outputErrAppend(char* rhs) {
	int ret_val;
		int rhs_file = open(rhs, O_WRONLY| O_APPEND | O_CREAT,
			 S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP );
	
	if(rhs_file == -1) {
		return -1;
	}

	// Redirect stdout to rhs
	original_desc = 1;
	new_desc = dup(original_desc);
	ret_val = dup2(rhs_file, 1);
	if(ret_val == -1) {
		return -1;
	}

	// Redirect stderr to rhs
	second_original_desc = 2;
	second_new_desc = dup(second_original_desc);
	ret_val = dup2(rhs_file, 2);
	if(ret_val == -1) {
		return -1;
	}
	return 0;	  
}

void restoreFileDescs() {
	if(original_desc != -1 && new_desc != -1) {
		dup2(new_desc, original_desc);
		original_desc = -1;
		new_desc = -1;
	}
	if(second_original_desc != -1 && second_new_desc != -1) {
		dup2(second_new_desc, second_original_desc);
		second_original_desc = -1;
		second_new_desc = -1;
	}
}

int performRedirect(char* my_redirect, char* my_token) {
	int ret_val = 0;
	char* lhs;
	char* rhs;
	char* token_copy;
	token_copy = strdup(my_token);

	// Two token redirections
	if(strcmp(my_redirect, "<") == 0 || strcmp(my_redirect, ">") == 0 || strcmp(my_redirect, ">>") == 0) {
		if(token_copy[0] == '>') {
			lhs = "1";
			rhs = strtok(token_copy, my_redirect);
		}
		else if(token_copy[0] == '<') {
			lhs = "0";
			rhs = strtok(token_copy, my_redirect);
		}
		else {
			lhs = strtok(token_copy, my_redirect);
			rhs = strtok(NULL, my_redirect);
		}
			
		//printf("LHS: %s\nRHS: %s\n", lhs, rhs);
		if(lhs == NULL || rhs == NULL) {
			ret_val = -1;
		}

		// Checking different redirs
		else if(strcmp(my_redirect,"<") == 0) {
			ret_val = inputRedirect(lhs,rhs);
		}
		else if(strcmp(my_redirect, ">") == 0) {
			ret_val = outputRedirect(lhs, rhs);
		}
		else {
			ret_val = outputAppend(lhs, rhs);
		}
	} 

	// One token redirection
	else if(strcmp(my_redirect, "&>") == 0 || strcmp(my_redirect, "&>>") == 0) {

		// No tokens on LHS
		if(token_copy[0] != '&') {
			ret_val =  -1;
		}
		else {
			
			rhs = strtok(token_copy, my_redirect);	
			if(rhs == NULL) { // Check for strtok error
				ret_val = -1;		
			}
			else if(strcmp(my_redirect, "&>") == 0) {
				ret_val = outputErrRedirect(rhs);
			}
			else {
				ret_val =  outputErrAppend(rhs);
			}
		}
	} 
	free(token_copy);
	return ret_val;
}

int runCommand(TokenArr* my_tokens) {
	char* my_command = my_tokens->tokens[0];
	char* path_val;
	int fork_val;
	int built_in_val = checkBuiltIn(my_command);
	
	switch(built_in_val) {

		// Non built in command
		case -1: 
	
			path_val = getPath(my_tokens);
			if(path_val == NULL) {
				fprintf(stderr, "Not a valid command\n");
				return -1;
			}

			fork_val = fork();

			// Parent
			if(fork_val > 0) { 
				addHistEntry(my_tokens);
				wait(NULL);
				if(path_val != NULL) {
					free(path_val);
				}
				return 0;
				
			}
			
			 // Child
			else if(fork_val == 0) {
				return execve(path_val, my_tokens->tokens, environ);
			}

			// ERROR
			else { 
				fprintf(stderr, "Error executing in child\n");
				return -1;
			}
			break;
			
		case EXIT: // exit

			// Checking for zero flags or parameters
			if(my_tokens->token_count > 1) {
				fprintf(stderr, "Error, exit should be used with no parameters\n");
				return -1;
			}
			else {
				freeTokenArr(my_tokens);
				wshExit();
			}
			break;
			
		case LS: // ls
		
			// Checking for zero flags or parameters
			if(my_tokens->token_count > 1) {
				fprintf(stderr, "Error, ls should be used with no parameters\n");
				return -1;
			}
			else{
				return wshLs();
			}
			break;
		case CD: // cd

			// Checking for exactly one arg
			if(my_tokens->token_count != 2) {
				fprintf(stderr, "Error, cd should be used with a single argument\n");
				return -1;
			}
			else {
				return wshCd(my_tokens->tokens[1]);
			}
			break;
			
		case EXPORT:
		case LOCAL: 
			// Always uses form export/local x=(y)
			if(my_tokens->token_count != 2) {
				fprintf(stderr, "Error, command should be of form export/local VAR=value\n");
				return -1;
			}
			if(my_tokens->tokens[1][0] == '=') {
				fprintf(stderr, "Error, can't have empty lhs in var assignment\n");
				return -1;
			}
		
			char* var_name;
			char* var_val;
			var_name = strtok(my_tokens->tokens[1], "="); 
			var_val = strtok(NULL, "=");
			
			
			// Check for strtok error
			if(var_name == NULL) {
				fprintf(stderr, "Error, Failed to assign var\n");
				return -1;
			}

			// Replace empty rhs with empty str
			if(var_val == NULL) {
				var_val = "";
			}

			// Putting these values into a token arr
			TokenArr* var_toks = malloc(sizeof(TokenArr));
			if(var_toks == NULL) {
				fprintf(stderr, "Error, Failed to assign var\n");
				return -1;
			}
			var_toks->token_count = 2;
			var_toks->tokens = malloc(2 * sizeof(char*));
			if(var_toks->tokens == NULL) {
				fprintf(stderr, "Error, Failed to assign var\n");
				return -1;
			}

			// Copy into tokenArr
			var_toks->tokens[0] = strdup(var_name);
			var_toks->tokens[1] = strdup(var_val);
			if(var_toks->tokens[0] == NULL || var_toks->tokens[1] == NULL) {
				fprintf(stderr, "Error, Failed to assign var\n");
				free(var_toks->tokens);
				free(var_toks);
				return -1;
			}

			// Substituting any vars
			if(substituteShellVars(var_toks) == -1) {
				fprintf(stderr, "Error, Failed to assign var\n");
				return -1;
			}

			// Reassign the tokens
			var_name = var_toks->tokens[0];
			var_val = var_toks->tokens[1];

			// Free the data struct holding the toks but not the tokens themselves
			free(var_toks->tokens);
			free(var_toks);
						
			int ret_val;
			if(built_in_val == LOCAL) {
				ret_val = wshLocal(var_name, var_val);
			}	
			else {
				ret_val = wshExport(var_name, var_val);
			}
			free(var_name);
			free(var_val);
			return ret_val;
			break;	
				
		case VARS: // vars

			// Checking vars is run on its own
			if(my_tokens->token_count != 1) {
				fprintf(stderr, "Invalid user of vars\n");
				return -1;
			}
			else {
				return wshVars();
			}
			break;	

		case HISTORY: // history

			// Prints list of previous commands
			if(my_tokens->token_count == 1) {
				return wshGetHist();
			}

			// Case where size is set
			else if(my_tokens->token_count == 3) {

				// Fail is second token isnt 'set'
				if(strcmp(my_tokens->tokens[1],"set") !=0) {
					fprintf(stderr, "Invalid input for history\n");
					return -1;
				}
				else {
				int my_val;
					my_val = atoi(my_tokens->tokens[2]);
					
					// History size cant be <=0 also handles atoi error
					if(my_val <= 0 ) { 
						fprintf(stderr, "Invalid input for history\n");
						return -1;
					}
					else {
						return wshSetHist(my_val);
					}
				}
			}
			else if(my_tokens->token_count == 2) {
				int my_val;
				my_val = atoi(my_tokens->tokens[1]);

				// Check size is > 0 and atoi retval is good
				if(my_val <=0 || my_val > histSize) {
					fprintf(stderr, "Invalid input for history\n");
					return -1;
				}
				else {
					struct HistEntry* my_entry = getHistEntry(my_val);
					return runCommand(my_entry->entry_tokens);
				}
			}
	}
	return 0;
}

int wshLocal(char* var_name, char* var_val) {
	char error_message[] = "Error adding shell var";
	struct ShellVar* shell_var_ptr;
	int var_loc;
	var_loc = findShellVar(var_name);
	
	// If the linked list is empty/has no entries
	if(shellLinkedListHead == NULL) {

		// Allocating head and tail
		shellLinkedListHead = malloc(sizeof(struct ShellVar));
		if(shellLinkedListHead == NULL) {
			fprintf(stderr, "%s\n", error_message);
			return -1;
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
			fprintf(stderr, "%s\n", error_message);
			return -1;
		}

		shell_var_ptr = shell_var_ptr->next_var; // Point to new var
		shell_var_ptr->next_var = NULL; // Set new vars next var to null
		shellLinkedListTail = shell_var_ptr; // Set new tail
	}
	
	// Copy vals into new var
	if(strcpy(shell_var_ptr->var_name, var_name) == NULL || strcpy(shell_var_ptr->var_val, var_val) == NULL) {
		fprintf(stderr, "%s\n", error_message);
		return -1;
	}
	return 0;	
}

int wshSetHist(int new_limit) {

	if(new_limit <= 0) {
		fprintf(stderr, "Error setting history size to %d\n", new_limit);
		return -1;
	}

	int size_diff;
	histLimit = new_limit;

	size_diff = histSize - histLimit;

	while(size_diff > 0) {
		removeHistEntry();
		size_diff = histSize - histLimit;
	}	
	return 0;
}

int wshGetHist() {
	struct HistEntry* hist_ptr = histHead;
	for(int i = 0; i < histSize; i++) {
		printf("%d) ", i + 1);
		for(int j = 0;j < hist_ptr->entry_tokens->token_count;j++) {
			printf("%s", hist_ptr->entry_tokens->tokens[j]);
			if(j != hist_ptr->entry_tokens->token_count -1 ) { // Only print space if not last token
				printf(" ");
			}
		}
		printf("\n");
		hist_ptr = hist_ptr->next_entry;
	}
	return 0;
}

int wshVars() {
	struct ShellVar* var_ptr;
	var_ptr = shellLinkedListHead;

	// Read until var is null
	while(var_ptr != NULL) {
		printf("%s=%s\n",var_ptr->var_name, var_ptr->var_val);
		var_ptr = var_ptr->next_var;
	}
	
	return 0;
}

void wshExit() {
	freeHistory();
	freeShellVars();
	exit(exit_global);
}

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
	
	while(dir_ret--) {
		free(my_dirent[dir_ret]);
	}
	free(my_dirent);
	
	return 0;
}

int wshCd(char* new_dir){
	if(chdir(new_dir) != 0) {
		printf("Error, could not cd to directory %s\n", new_dir);
		return -1;
	}
	return 0;
}

int wshExport(char* var_name, char* var_val) {
	int ret_val;
	ret_val = setenv(var_name, var_val, 1); // Change and overwriting

	if(ret_val == -1) {
		fprintf(stderr, "Error setting environment variable\n");
		return -1;
	}
	return 0;
	
}

int main(int argc, char* argv[]) {
	FILE* sh_file;
	wshExport("PATH", "/bin");
	if(argc == 1) {
		programLoop(stdin);
	}
	else if(argc == 2) {
		sh_file = fopen(argv[1], "r");
		if(sh_file != NULL) {
			programLoop(sh_file);
		}
		else {
			fprintf(stderr, "Error running shell file\n");
		}
	}
	else {
		fprintf(stderr, "Wsh can only be run with 0 or 1 params\n");
	}
}

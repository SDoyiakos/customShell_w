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
 *Test function merely for me to see if tokens are being grabbed properly
 **/
void debugTokens(TokenArr* my_tokens) {
  printf("The tokens in the arr are:\n");
  for(int i = 0;i < my_tokens->token_count;i++) {
    printf("%s\n", my_tokens->tokens[i]);
  }
}

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
	for(int i = 0;i< my_tokens->token_count;i++) {
		if(my_tokens->tokens[i][0] == '$') { // If token is a var
			my_var = &my_tokens->tokens[i][1]; // Retrieve str without the $
			my_var = getShellVar(my_var); // Get the vars value

			// Replacing the token with the var's value
			free(my_tokens->tokens[i]);
			my_tokens->tokens[i] = malloc(strlen(my_var) + 1);
			if(my_tokens->tokens[i] == NULL) {
				printf("Malloc error\n");
				return -1;
			}
			strcpy(my_tokens->tokens[i], my_var);
		}
	}
	return 0;
}

void programLoop(FILE* input_stream) {
	TokenArr* my_tokens;
	char* user_input = malloc(SHELL_MAX_INPUT * sizeof(char));
	int* input_size = malloc(sizeof(int));

	// Run loop until exit
	while(1) {
		if(input_stream == stdin) {
			printf("wsh> ");
			fflush(stdout);
		}	
		
		parseInputs(user_input, input_size, input_stream);	
		my_tokens = NULL;
		// Check for EOF after getting input
		if(feof(input_stream)) {
			wshExit();
		}

		if(*input_size != 0) {
			my_tokens = tokenizeString(user_input); // Tokenize input
			if(my_tokens->tokens[0][0] != '#') {				
				substituteShellVars(my_tokens);
				runCommand(my_tokens);
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
	if(arr1->token_count != arr2->token_count) {
		return 0;
	}
	for(int i = 0; i < arr1->token_count;i++) {
		if(strcmp(arr1->tokens[i],arr2->tokens[i]) != 0) {
			return 0;
		}
	}
	return 1;
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
	my_copy->tokens = malloc( (my_copy->token_count + 1) * sizeof(char*));
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
		strcpy(my_copy->tokens[i], my_tokens->tokens[i]);
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

char* getPath(TokenArr* my_tokens) {
	char error_message[] = "Error getting path";
	int acc_val;
	char* command_cpy;
	command_cpy = malloc(strlen(my_tokens->tokens[0]) + sizeof(char*));
	if(command_cpy == NULL) {
		return NULL;
	}
	strcpy(command_cpy, my_tokens->tokens[0]);
	
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
			return NULL;
		}
		if(strcpy(path_str, getenv("PATH")) == NULL) {
			fprintf(stderr, "%s\n", error_message);
			return NULL;
		}

		// Allocate mem for the dir
		full_dir_ptr = malloc(sizeof(char) * MAX_DIR_SIZE);
		if(full_dir_ptr == NULL) {
			fprintf(stderr, "%s\n", error_message);
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
		free(path_str);
		free(command_cpy);
		return NULL; // No path found
	}
	
}

int runCommand(TokenArr* my_tokens) {
	char* my_command = my_tokens->tokens[0];
	char* path_val;
	int fork_val;
	switch(checkBuiltIn(my_command)) {

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

			// Export always uses form export x=(y)
			if(my_tokens->token_count != 2) {
				fprintf(stderr, "Error, export should be used in the manner, export VAR=value\n");
				return -1;
			}
			else {
				char* var_name;
				char* var_val;
				var_name = strtok(my_tokens->tokens[1], "="); 
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

		case LOCAL: // Local

			// Always uses form local x=(y)
			if(my_tokens->token_count !=2) {
				printf("Invalid input for local\n");
				return -1;
			}
			else {
			
				char* var_name;
				char* var_val;
				var_name = strtok(my_tokens->tokens[1], "=");
				var_val = strtok(NULL, "=");

				// If RHS is a variable
				if(var_val != NULL && var_val[0] == '$') {
					var_val = getShellVar(&var_val[1]);
				}
					
				// If var_name is null or a variable reference
				if(var_name == NULL || var_name[0] == '$') {
						fprintf(stderr, "Error, export failed to assign shell var\n");
						return -1;
				}
				else {
					return wshLocal(var_name, var_val);
				}
			}
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

	if(var_val == NULL) {
		var_val = "";
	}
	
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
	exit(0);
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

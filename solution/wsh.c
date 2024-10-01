#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "wsh.h"


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
			strcpy(my_tokens.tokens[i], token_buf);
		}
	}
	return my_tokens;	
}

void parseInputs(char* input_buffer, int* input_size) {
	
	size_t buffer_len = MAX_INPUT; // The size of the buffer
	
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
}


int interactiveMode() {
	TokenArr my_tokens;
	char* user_input = malloc(MAX_INPUT * sizeof(char));
	int* input_size = malloc(sizeof(int));

	// Run loop until exit
	while(1) {
		printf("wsh> ");
		parseInputs(user_input, input_size);
		printf("MY STR: %s\n", user_input);
		my_tokens = tokenizeString(user_input, *input_size); // Tokenize input
		debugTokens(my_tokens);
		
	}	
}



int main(int argc, char* argv[]) {
	if(argc == 1) {
		interactiveMode();
	}
}

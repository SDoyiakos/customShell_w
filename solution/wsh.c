#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "wsh.h"

#define MAX_INPUT 1024

TokenArr tokenizeString(char* my_str, int input_size) {
	int token_count;

	// Allocated char count + 1 as we need the /0
	char* my_str_cpy = malloc(1 + (input_size * sizeof(char)));
	if(my_str_cpy == NULL) {
		exit(-1);
	}

	// Copy string
	if(strcpy(my_str_cpy, my_str) == NULL) {
		exit(-1);
	}
	
}

void parseInputs(char* input_buffer, int* input_size) {
	
	size_t line_length = MAX_INPUT;
	
	// Getting next user input
	*input_size = getline(&input_buffer, &line_length, stdin);

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
	char* user_input = NULL;
	int* input_size = malloc(sizeof(int));
	while(1) {
		printf("wsh> ");
		parseInputs(user_input, input_size);

		// Tokenize input
		
	}	
}



int main(int argc, char* argv[]) {
	if(argc == 1) {
		interactiveMode();
	}
}

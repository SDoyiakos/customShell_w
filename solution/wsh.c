#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_INPUT 1024

void parseInputs(char* input_buffer, int* input_size) {
	
	size_t line_length = MAX_INPUT;
	
	// Getting next user input
	*input_size = getline(&input_buffer, &line_length, stdin);

	// Checking whether input has errors or not
	if(*input_size == -1) {
		if(errno == EINVAL || errno == ENOMEM) {
			printf("Error reading new line\nExiting\n");
			exit(-1);
		}
	}
	// Checking for exit command
	if(strcmp(input_buffer, "exit\n") == 0) {
		printf("Exiting\n");
		exit(0);
	}
}


int interactiveMode() {
	char* user_input = NULL;
	int* input_size = malloc(sizeof(int));
	while(1) {
		printf("wsh> ");
		parseInputs(user_input, input_size);
		free(user_input);
	}	
}



int main(int argc, char* argv[]) {
	if(argc == 1) {
		interactiveMode();
	}
}

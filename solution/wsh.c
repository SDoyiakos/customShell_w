#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int interactiveMode() {
	size_t line_length = 1024;
	char* user_input;
	while(1) {
		printf("wsh> ");

		// Getting next user input
		if(getline(&user_input, &line_length, stdin) == -1) {
			if(errno == EINVAL || errno == ENOMEM) {
				exit(-1); // Error Exit
			}
			exit(0); // EOF Exit
		}

		// Checking for exit command
		if(strcmp(user_input, "exit\n") == 0) {
			exit(0);
		}
	}	
}



int main(int argc, char* argv[]) {
	if(argc == 1) {
		interactiveMode();
	}

	
	
}

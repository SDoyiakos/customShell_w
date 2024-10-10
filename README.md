Name: Steven Doyiakos
Email: sdoyiakos@wisc.edu
CS Login: doyiakos
ID: 908-322-2274
Status: Complete & Functioning


Implementation:

This is the README which describes the implementation of wsh

[Execution Order]
The wsh runs in a sequential order of the following manner
	1. Check for a file arg when running wsh and update stream accordingly
	2. Until EOF or exit is seen, read the next line from the input stream
	3. Break the string up into its individual tokens delimited by a space
	4. Iterate over all tokens and update their values if they're a dereference variable with $
	5. Using the first token, determine which built in shell command or other command is to be run
	6. Sanitize the inputs to the command
	7. Go to step 1

[TokenArr Implementation]
	- The TokenArr is how wsh stores all of its tokens for a given command
	- Tokens are retrieved using strtok and then are copied into TokenArr.tokens
		- We copy the vals in order to have them on the heap which makes freeing more uniform
	- When retrieving user input, the initial size limit of TokenArr.tokens is 10 and is consistently reallocated by 1 for each entry over that limit
	- When retrieving user input, we also reserve the last entry in TokenArr.tokens to be NULL as it makes passing these tokens as args easier in execve

[History Implementation]
	- The history is implemented via a doubly linked list
	- All entries are added to the front of this list
	- All entries are removed from the end of the list
	- When resizing the history array, entries are removed until the size reaches the upper limit

[Variables Implementation]
	- The shell variables are implemented in a linked list
	- All variables are added by being appened to the end of the list to maintain the added order
	- When a variable is searched for, first it is searched for in env, and then if its not found, we do a linear search in the linked list


[Redirect Implementation]
	- Redirects are implemented by checking the last token of the TokenArr
	- We figure out what redirect operation is to be done by using strstr which finds the substring in the token
	- When then sanitize and check for proper usage of the redirect
	- We jump into one of the many redirect functions
	- Once the command has been performed, we run restoreFileDescs, which make use of 
	  global ints which can be used to restore file descriptors to their original stream


[Exiting]
	- The program does the following when encountering EOF or 'exit'
	- The history and shell vars are cleared from memory
	- Memory for the most recent command is free'd
	- The program calls the syscall exit with the rc of the most recent command execution


[Spaces for Improvement]
	- Repeated code
		- Functions such as some ofthe redirections or getShellVar and findShellVar could be combined into singular functions
		- The ShellVar and HistEntry structs are very similar and there are ways to have needed a single struct def for the implementations

	- Expandability
		- This code can easily break if not expanded correctly
		- For instance, the COMMAND_ARR is used when discovering what command is run. If the defines for the indices are wrong, the program performs unexpectedly.

	- TokenArr Creation
		- TokenArr is allocated with a tokens of size 10 and if all 10 arent used then that space is left allocated leading a bit of internal fragmentation
		- TokenArr when growing beyond size 10 will have to do a realloc for each token which is costly


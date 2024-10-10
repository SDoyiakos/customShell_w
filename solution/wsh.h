#include <stdio.h>
#define SHELL_MAX_INPUT 1024
#define MAX_DIR_SIZE 1024

#define EXIT 0
#define LS 1
#define CD 2
#define EXPORT 3
#define LOCAL 4
#define VARS 5
#define HISTORY 6

// Struct acts as a node in a linked list
struct ShellVar {
	char var_name[SHELL_MAX_INPUT];
	char var_val[SHELL_MAX_INPUT];
	struct ShellVar* next_var;
};

// Struct for tokenized user inputs
typedef struct {
	int token_count;
	char** tokens;
} TokenArr;

struct HistEntry {
	TokenArr* entry_tokens;
	struct HistEntry* next_entry;
	struct HistEntry* prev_entry;
};

// Commands must be added to end to preserve indices
const char* COMMAND_ARR[] = 
{
	"exit",
	"ls",
	"cd",
	"export",
	"local",
	"vars",
	"history"
};

// BUILT IN FUNCTIONS

/**
* Shell built in exit function
* Calls exit() syscall
**/
void wshExit();

/**
* Shell built in ls function
* Performs like bash's ls -1
**/
int wshLs();

/**
*	Shell change directory command.
*	Takes param to new dir 
**/
int wshCd(char* new_dir);

/**
* Sets environment variable var_name=var_val
**/
int wshExport(char* var_name, char* var_val);

/**
* Built in wsh command. Adds the variable to the
* local vars array. Updates value instead if already found in local vars array
**/
int wshLocal(char* var_name, char* var_val);

/**
* Prints all shell vars
**/
int wshVars();

/**
* Built in command that prints the history of external commands
**/
int wshGetHist();

/**
* Built in command that sets the size of the history list
**/
int wshSetHist(int new_size);


// Internal shell functions

/**
* Gets the index of a built in command in the COMMAND_ARR
* Returns -1 if command isnt built in
**/
int checkBuiltIn(char* my_command);

/**
* Retrieves the variable value associated with the variable name.
* If the variable is not found then return empty string
**/
char* getShellVar(char* var_name);

/**
* Gets the index of a shell variable denoted by
* the var_name
**/
int findShellVar(char* var_name);

/**
* Frees the memory allocated by the my_tokens variable.
* my_tokens and its contents must be allocated
**/
void freeTokenArr(TokenArr* my_tokens);

/**
* Runs the program indefinitely until exit or eof
**/
void programLoop(FILE* input_stream);

/**
* Replaces any shell vars in the tokens with their variable value
**/ 
int substituteShellVars(TokenArr* my_tokens);

/**
* Retrieves the next line in the program.
* The retrieved line and its length are stored within
* *input_buffer and *input_size respectively
**/
int parseInputs(char* input_buffer, int* input_size, FILE* input_stream);


/**
* Takes in two TokenArrs, arr1 and arr2, and returns 0 if ne and 1 if equal
**/
int tokenCmp(TokenArr* arr1, TokenArr* arr2);

/**
* Creates a copy of the TokenArr* including a tokens** field which points
* to a separate region in memory
**/
TokenArr* copyTokenArr(TokenArr* my_tokens);

/**
* Pushes entry into the command history list.
* If input exceeds the size limit, removes the last entry
**/
int addHistEntry(TokenArr* my_tokens);

/**
* Returns the HistEntry ptr to the entry at the given index
**/
struct HistEntry* getHistEntry(int index);

/**
* Determines which command is going to be run
* If command is wsh built-in command, input is checked and/or sanitized
**/
int runCommand(TokenArr* my_tokens);

/**
* Removes the tail entry of the history list
**/
void removeHistEntry();

/**
* Retrieves the path to a program in the following order.
* 1. Looks for a relative path.
* 2. Looks for a full path.
* 3. Looks for a path within $PATH
**/
char* getPath(TokenArr* my_tokens);

/**
* Separates the input across token '='.
* Returns a TokenArr of the inputs
**/
TokenArr* tokenizeString(char* my_str);

/**
* Returns the symbol(s) associated with the redirect token
* NULL if no redirect is done
**/
char* getRedirect(char* my_token);

/**
* Performs the given redirect action
**/
int performRedirect(char* my_redirect, char* my_token);

/**
* Sets rhs for reading on the lhs file desc
**/
int inputRedirect(char* lhs, char* rhs);

/**
* Sets rhs for writing on lhs file desc
**/
int outputRedirect(char* lhs, char* rhs);

/**
* Set rhs for append writing on lhs file desc
**/
int outputAppend(char* lhs, char* rhs);

/**
* Prints stdout and stderr to rhs file
**/
int outputErrRedirect(char* rhs);

/**
* Appends stdout and stderr to rhs file
**/
int outputErrAppend(char* rhs);

/**
* Restores all file descriptors to their original values
**/
void restoreFileDescs();

/**
*  Frees all entries in the history arr
**/
void freeHistory();

/**
* Frees all entries in the shell var arr
**/
void freeShellVars();

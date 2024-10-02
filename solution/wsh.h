
#define SHELL_MAX_INPUT 1024
#define MAX_DIR_SIZE 1024

typedef struct {
	char* var_name;
	char* var_val;
} ShellVar;


// Struct for tokenized user inputs
typedef struct {
	int token_count;
	char** tokens;
} TokenArr;

// Commands must be added to end to preserve indices
const char* COMMAND_ARR[] = 
{
	"exit",
	"ls",
	"cd",
	"export",
	"local",
	"vars"
};


// BUILT IN FUNCTIONS
void wshExit();
void wshLs();
void wshCd(char* new_dir);
void wshExport(char* var_name, char* var_val);
void wshVars();

int checkBuiltIn(char* my_command);
void runCommand(TokenArr my_tokens);
TokenArr tokenizeString(char* my_str, int input_size);

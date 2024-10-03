
#define SHELL_MAX_INPUT 1024
#define MAX_DIR_SIZE 1024

// Struct acts as a node in a linked list
struct ShellVar {
	char var_name[SHELL_MAX_INPUT];
	char var_val[SHELL_MAX_INPUT];
	struct ShellVar* next_var;
};
void insertShellVar(char* var_name, char* var_val);
char* getShellVar(char* var_name);

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
void wshLocal(char* var_name, char* var_val);
void wshVars();


// Internal shell functions
char* getBuiltIn(char* my_command);


void runCommand(TokenArr my_tokens);
TokenArr tokenizeString(char* my_str, int input_size);


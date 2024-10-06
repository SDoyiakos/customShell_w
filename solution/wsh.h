
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

struct HistEntry {
	char command[SHELL_MAX_INPUT];
	struct HistEntry* next_entry;
	struct HistEntry* prev_entry;
};
void addHistEntry(char* command);
void removeHistEntry();

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
	"vars",
	"history"
};

// BUILT IN FUNCTIONS
void wshExit();
int wshLs();
void wshCd(char* new_dir);
void wshExport(char* var_name, char* var_val);
void wshLocal(char* var_name, char* var_val);
void wshVars();
void wshGetHist();
void wshSetHist(int new_size);


// Internal shell functions
char* getBuiltIn(char* my_command);


int runCommand(TokenArr my_tokens, char* user_input);
TokenArr tokenizeString(char* my_str, int input_size);


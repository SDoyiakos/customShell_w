
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

struct HistEntry {
	TokenArr* entry_tokens;
	struct HistEntry* next_entry;
	struct HistEntry* prev_entry;
};
int addHistEntry(TokenArr* my_tokens);
void removeHistEntry();


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
int wshCd(char* new_dir);
int wshExport(char* var_name, char* var_val);
int wshLocal(char* var_name, char* var_val);
int wshVars();
int wshGetHist();
int wshSetHist(int new_size);


// Internal shell functions
char* getBuiltIn(char* my_command);
void freeTokenArr(TokenArr* my_tokens);


int runCommand(TokenArr* my_tokens);
TokenArr* tokenizeString(char* my_str);


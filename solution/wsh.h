
#define MAX_INPUT 1024


typedef struct {
	int token_count;
	char** tokens;
} TokenArr;

// Commands must be added to end to preserve indices
const char* COMMAND_ARR[] = 
{
	"exit"
};


// BUILT IN FUNCTIONS
void wshExit();


int checkBuiltIn(char* my_command);
void runCommand(TokenArr my_tokens);
TokenArr tokenizeString(char* my_str, int input_size);

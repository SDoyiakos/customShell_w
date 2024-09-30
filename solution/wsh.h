typedef struct {
	int token_count;
	char** tokens;
} TokenArr;

void wsh_exit();
TokenArr tokenizeString(char* my_str, int input_size);

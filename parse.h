#ifndef nshparse
#define nshparse
#include <string.h>
#include <stdlib.h>

#define CMD_LINE_BUF_SIZE 1024
#define MAX_ARG_COUNT 128

typedef struct parse_struct{
	char buf[CMD_LINE_BUF_SIZE]; // copy of the command line string
	int tokencount; 			 // number of tokens in this struct
	char* tokens[MAX_ARG_COUNT]; // array of pointers to the buffer for each token, in logical order
	char* commands[MAX_ARG_COUNT]; // array of pointers to the buffer for each command
	int commandcount; // number of commands in command array
	char* inputredir; // NULL if no input redirection, otherwise yields the infile
	char* outputredir; // NULL if no output redir, otherwise yields the outfile
	char** commandargs[MAX_ARG_COUNT]; //array of pointers to arrays of arguments for each command
} parse_st;



int parse(parse_st* st,char* buf);

void cleanup_parse_st(parse_st* st);












#endif
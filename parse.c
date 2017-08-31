#include "parse.h"

void cleanup_parse_st(parse_st* st){
	int i;
	for (i = 0; i < st->commandcount; i++){ // free command args arrays
		free(st->commandargs[i]);
	}
	for (i = 0; i < st->tokencount; i++){ // free tokens
		free(st->tokens[i]);
	}
}

int Parse(parse_st* st,char* buf){
	// initialize structure
	char buftemp[CMD_LINE_BUF_SIZE]; // a buffer that strtok can modify before we sort the tokens out nicely
	strcpy(buftemp,buf);
	st->tokencount = 0;
	st->commandcount=0;
	st->inputredir = NULL;
	st->outputredir = NULL;
	//

	// basic tokenization
	char * tokens[MAX_ARG_COUNT]; //storing the unordered tokens
	int tokencount=0; // count of unordered token array
	int i = 0;
	int tok = 0; // flag to determine if a token has been assigned yet
	char *s;

	s = strtok(buftemp," \t\n\r");
	while(s != NULL){
		tokencount++;
		tokens[i] = strdup(s);
		s = strtok(NULL," \t\n\r");
		i++;
	}

	//parse into logical order, temporarily removing the infile outfile and > < from the token ordering
	char * tokenslogical[MAX_ARG_COUNT]; // pointer array for next sorting step
	int tokenslogicalcount = 0;
	char * inputredirsym = NULL; // temporarily hold the pointer to the < token
	char * outputredirsym = NULL;// temporarily hold the pointer to the > token
	int commandfound = 0; // to track whether the command has been found in the subset of tokens
	int cargsarraysize = 0; // track array size to tell if we need to realloc a larger array to store argument pointers
	int cargcount = 0;		// track how many args in the carg array
	for (i=0;i < tokencount;i++){
		if ((*tokens[i]) == '#'){ // comment char, stop parsing
			for (;i<tokencount;i++){ // finish the loop without looking at the rest of the tokens
				tokenslogical[tokenslogicalcount++] = tokens[i];
			}
		}
		else if ((*tokens[i]) == '<') {
			inputredirsym = tokens[i];
			st->inputredir = tokens[++i]; // skip the parsing of the next token, it must be the infile name
		}
		else if ((*tokens[i]) == '>') {
			outputredirsym = tokens[i];
			st->outputredir = tokens[++i]; // skip the parsing of the next token, it must be the outfile name
		}
		else if ((*tokens[i]) == '|') {
			commandfound = 0; // the next word we find is a command on the other end of the pipe
			tokenslogical[tokenslogicalcount++] = tokens[i];
			st->commandargs[st->commandcount-1][cargcount] = NULL; // null terminate our carg array, since we've finished
		}
		else
		{	// assume word has been found
			if (commandfound == 0){// first token in a subsequence is the command
				st->commands[st->commandcount] = tokens[i];
				tokenslogical[tokenslogicalcount++] = tokens[i];
				commandfound = 1;
				cargcount = 0;
				cargsarraysize = 4;
				st->commandargs[st->commandcount] = malloc(cargsarraysize*sizeof(char*));
				if (st->commandargs[st->commandcount] == NULL){
					perror("nsh: couldn't allocate memory while parsing commandline");
					exit(1);
				}
				st->commandargs[st->commandcount][cargcount++] = tokens[i]; // execvp expects the first token to be the name of the command
				st->commandcount++;
			}
			else{ // if we've already found the command for this subset, then this is an argument token
				tokenslogical[tokenslogicalcount++] = tokens[i];
				if (cargcount >= cargsarraysize-2){
					cargsarraysize = cargsarraysize*2 + 1;// expand carg array size before we exceed it
					st->commandargs[st->commandcount-1] = realloc(st->commandargs[st->commandcount-1], cargsarraysize*sizeof(char*));
					if (st->commandargs[st->commandcount-1]== NULL){
						perror("nsh: couldn't allocate memory while parsing commandline");
						exit(1);
					}
				}
				st->commandargs[st->commandcount-1][cargcount++] = tokens[i]; // add token to the argument array for the current command
			}
		
	
		}
	}
	if ( st->commandcount > 0){
		st->commandargs[st->commandcount-1][cargcount] = NULL; // null terminate the final command argument group that was parsed;
	}


	// prepend and append input redirection tokens to form the final ordering
	if (st->inputredir != NULL){ // prepend
		st->tokens[0] = inputredirsym;
		st->tokens[1] = st->inputredir;
		st->tokencount = 2;
	}
	else 
		st->tokencount = 0;

	for (i=0;i < tokenslogicalcount;i++){ // fill final token array with the ordered tokens
		st->tokens[st->tokencount++] = tokenslogical[i];
	}
	if (st->outputredir != NULL) { //append
		st->tokens[st->tokencount] = outputredirsym;
		st->tokens[st->tokencount+1] = st->outputredir;
		st->tokencount +=2;
	}

	return 0;
}


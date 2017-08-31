#include <string.h>
#include <stdio.h>
#include "parse.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


void print_parse_st(parse_st* st);
void print_parse_st_verbose(parse_st* st);
int spawnchildren(parse_st* st);
void print_prompt();


void print_parse_st(parse_st* st){
	printf("%d: ",st->commandcount);

	int i;
	int prevneedsspace; // when to put a space for formatting between words
	for (i = 0; i < st->tokencount; i++){
		if (((*st->tokens[i]) != '<') && ((*st->tokens[i]) != '>') && ((*st->tokens[i]) != '|')){
			printf("'");
			printf(st->tokens[i]);
			printf("'");
			prevneedsspace = 1;
		}
		else{
			prevneedsspace = ((*st->tokens[i]) == '|') ? 1 : 0;
			printf(st->tokens[i]);
		}
		if (prevneedsspace!=0)
			printf(" "); // space for fomatting between words
	}
	printf("\n");
}

void print_parse_st_verbose(parse_st* st){
	printf("commandcount: %d\n",st->commandcount);
	printf("tokencount: %d\n",st->tokencount);

	int i,j;
	int prevneedsspace; // when to put a space for formatting between words
	for (i = 0; i < st->tokencount; i++){
		if (((*st->tokens[i]) != '<') && ((*st->tokens[i]) != '>') && ((*st->tokens[i]) != '|')){
			printf("'");
			printf(st->tokens[i]);
			printf("'");
			prevneedsspace = 1;
		}
		else{
			prevneedsspace = ((*st->tokens[i]) == '|') ? 1 : 0;
			printf(st->tokens[i]);
		}
		if (prevneedsspace!=0)
			printf(" "); // space for fomatting between words
	}
	for (i = 0; i < st->commandcount; i++){ // print command list
			printf("\nargs: ");
			j = 0;
			while(st->commandargs[i][j] != NULL){
				printf("'%s' ",st->commandargs[i][j]);
				j++;
			}
			prevneedsspace = 1;
			printf(" "); // space for fomatting between words
	}

	if (st->inputredir != NULL)
		printf("\ninfile: %s ", st->inputredir);
	if (st->outputredir != NULL)
		printf("\noutfile: %s", st->outputredir);
	printf("\n");
}

int spawnchildren(parse_st* st){
	int numofchildren = 0;
	int pid, i, fd;
	int pipefdprev; // hold the previous write end of the pipe for the next process
	int pipefd[2]; // 

	char ispipeopen=0;
	char isprevpipeopen=0;
	char isfirstcommand=0; // vars to store booleans for readability
	char islastcommand=0;
	for (i = 0; i < st->commandcount; i++) // spawn 1 child per command
	{
		isfirstcommand = ( i == 0 );
		islastcommand = ( i == (st->commandcount-1) );

		if ( (st->commandcount > 1) && (! islastcommand) ){ 
		// if we have more than 1 command, and this isn't the last command in the pipe
			if (pipe(pipefd) == -1){
				perror("nsh: error creating pipe");
				return -1;
			}
			ispipeopen = 1;
		}
		// handle builtin commands within this block before the fork/exec pair////////////////////////////////////////////////////////////
		if (strcmp(st->commands[i],"cd") == 0 ){ // CD
			if ( chdir(st->commandargs[i][1]) == -1 ){
				perror("nsh: error changing directory");
				return -1;
			}
			continue;
		}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pid = fork();
		if (pid == 0){ //child process
			if (ispipeopen)
				close(pipefd[0]); // no use for this process to have the read end
			//perform io redirrects, if necessary
			if (isfirstcommand){ // input redirect would only be on the first command
				if (st->inputredir != NULL){
					// perform input redir to file
					fd = open(st->inputredir,O_RDONLY);
					if (fd == -1){
						perror("nsh: error opening input file");
						return -1;
					}
					if ( dup2(fd,0) == -1 ){
						perror("nsh: error calling dup on input fd");
						return -1;
					}
					close(fd);
				}
				if (ispipeopen){
					if ( dup2(pipefd[1],1) == -1) { // dup pipe write end onto the stdout fd
						perror("nsh: error calling dup on fd");
						return -1;
					}
				}
			}
			if (islastcommand){ // output redirect would only be on the last command
				if (st->outputredir != NULL){
					//perform output redir to file
					fd = open(st->outputredir,O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
					if (fd == -1){
						perror("nsh: error opening output file");
						return -1;
					}
					if ( dup2(fd,1) == -1 ){
						perror("nsh: error calling dup on output fd");
						return -1;
					}
					close(fd);
				}
				if (isprevpipeopen){
					if ( dup2(pipefdprev,0) == -1) { // dup pipe read end onto the stdin
						perror("nsh: error calling dup on fd");
						return -1;
					}
				}
			}
			if( (!isfirstcommand) && (!islastcommand) ){ // the 'middle' commands have pipes on both ends
				if ( dup2(pipefdprev,0) == -1) { // dup pipe read end onto the stdin
					perror("nsh: error calling dup on fd");
					return -1;
				}
				close(pipefdprev);
				if ( dup2(pipefd[1],1) == -1) { // dup pipe write end onto the stdout fd
					perror("nsh: error calling dup on fd");
					return -1;
				}
			}
			if (isprevpipeopen)
				close(pipefdprev);// dups are complete, close fd
			if (ispipeopen)
				close(pipefd[1]); // dups are complete, close fd
			// exec the command
			if (execvp(st->commands[i],st->commandargs[i]) == -1){
				perror("nsh: error cannot execvp command");
				return -1;
			}

		}
		// end child code
		else if (pid > 0){ // parent process
			numofchildren++;
			if (isprevpipeopen){
				close(pipefdprev); // the read end of the previous pipe should be closed now
				isprevpipeopen= 0;
			}
			if (ispipeopen) {
				close(pipefd[1]); // close the write end of the pipe made this cycle
				pipefdprev = pipefd[0];// hang onto the the pipe for the next spawned child
				ispipeopen=0;
				isprevpipeopen =1; // once we know we have a previous pipe, need to start closing it
			}
		}
		else{ // error case
			perror("nsh: error spawning child process");
			return -1;
		}
	}
	return numofchildren;
}


void print_prompt(){
	printf("? ");
	return;
}

int main(int argc, char** argv){
	char buf[CMD_LINE_BUF_SIZE];
	parse_st st;
	int childcount; // track how many child processes we should wait on
	int childstatus;
	int isinteractive = 1;
	if (argc > 1){ // run scripts
		int fd = open(argv[1],O_RDONLY);
		isinteractive = 0;
		if (fd == -1){
			perror("nsh: error opening script given on argv[1]");
			return 1;
		}
		if (dup2(fd,0) == -1){ // dup file fd to stdin
			perror("nsh: error duping script fd onto stdin");
			return 1;
		}
	}
	while(1){
		if ( isinteractive ) // only print prompt if we're not executing a script
			print_prompt();
		if (fgets(buf, CMD_LINE_BUF_SIZE - 1 ,stdin) != NULL){
			if (ferror(stdin)){
				perror("nsh: error reading on stdin");
				return 1;
			}
			Parse(&st,buf);
			// print_parse_st(&st);
			// print_parse_st_verbose(&st);
			childcount = spawnchildren(&st);
			if ( childcount == -1){
				exit(1);
			}
			for(;childcount > 0; childcount--){
				if (wait(&childstatus) == -1 ){
					perror("nsh: error waiting for child process to terminate");
					exit(2);
				}
			}
			cleanup_parse_st(&st);
			if (feof(stdin))
				return 0;

		}
		else
			break;
	}

	return 0;
}
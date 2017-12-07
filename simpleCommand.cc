#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "simpleCommand.hh"

void myunputc(int c)
;

SimpleCommand::SimpleCommand() {
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void SimpleCommand::insertArgument( char * argument ) {
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;

	//if (_numOfArguments == 1)
		//setenv("_", strdup(argument), 1);
}

void SimpleCommand::subShell(char * command) {
	int tmpin=dup(0);
	int tmpout=dup(1);

	size_t len = strlen(command) + 6;
	char * newCommand = (char *) malloc(sizeof(char) * len);
	strcpy(newCommand, command);
	strcat(newCommand, "\nexit\n");
	int fdpipe[2];
	int fdpipe2[2];
	pipe(fdpipe);
	pipe(fdpipe2);
	int fdout = fdpipe[1];
	int fdin = fdpipe[0];
	write(fdout, (void *) newCommand, len);
	close(fdout);
	int ret;
	ret = fork();
	if (ret == 0) {
		//child
		dup2(fdin, 0);
		close(fdpipe[1]);
		close(fdpipe[0]);
		dup2(fdpipe2[1], 1);
		close(fdpipe2[1]);
    	execvp("/proc/self/exe", NULL);
      	perror("execvp");
		_exit(1);
	} else if (ret < 0) {
		perror("fork");
		return;
	}
	close(fdin);
	int m = 256;
	int i = 0;
	char * buffer  = (char *) malloc(sizeof(char) * m);
	read(fdpipe2[0], (void *) buffer, m);
	len = strlen(buffer);
	for (i = len - 1; i >= 0; i--) {
		myunputc(buffer[i]);
	}
	myunputc(' ');
	close(fdpipe2[0]);

	dup2(tmpin,0);
	dup2(tmpout,1);
}

void SimpleCommand::insertEnv( char * argument ) {
	char ** p = environ;
	if (strcmp(argument, "$") == 0) {
		int pid = getpid();
		char pidStr[8];
		sprintf(pidStr, "%d", pid);
		insertArgument(strdup(pidStr));
	/*} else if (strcmp(argument, "!") == 0) {
		int pid = 0;
		char pidStr[8];
		sprintf(pidStr, "%d", pid);
		insertArgument(strdup(pidStr));*/
	} else {
		while (*p != NULL) {
			char * var = strtok(*p, "=");
			char * val = strtok(NULL, "=");
			if (strcmp(var, argument) == 0) {
				insertArgument(strdup(val));
				break;
			}
			p++;
		}
	}
}

void SimpleCommand::sort(int i, int j) {
	qsort(&_arguments[i], j - i, sizeof(char *), cmpstr);
}



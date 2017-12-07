/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "command.hh"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_appendErr = 0;
	_pid = -1;
	backgroundProcs = (int *) malloc(sizeof(int) * 10);
	backgroundIndex = 0;

}

int Command::isBackgroundProc(int pid) {
	for (int i = 0; i <= backgroundIndex; i++) {
		if (backgroundProcs[i] == pid)
			return true;
	}
	return false;
}

void Command::removeBackgroundProc(int pid) {
	for (int i = 0; i <= backgroundIndex; i++) {
		if (backgroundProcs[i] == pid)
			backgroundProcs[i] = -1;
	}
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void Command::insertOutput( char * out ) {
	if (_outFile) {
		perror("Ambiguous output redirection");
		clear();
	} else {
		_outFile = out;
	}
}

void Command:: clear() {
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		if (_outFile != _errFile) {
			free( _outFile );
		}
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_appendErr = 0;
	_pid = -1;
}

void Command::exitShell() {
	clear();
	printf("Ciao!\n");
	exit(0);
}

void Command::print() {
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ------------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background   Append\n" );
	printf( "  ------------ ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO",
		_append?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	//print();

	// Add execution here
	//Save in / out
	int tmpin=dup(0);
	int tmpout=dup(1);
	int tmperr=dup(2);

	//set the initial input
	int fdin;
	if (_inFile) {
    	fdin = open(_inFile, O_RDONLY);
  	}
  	else {
    	// Use default input
    	fdin = dup(tmpin);
	}
	
	int fderr;
	if (_errFile) {
		if (_appendErr) {
			fderr = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0660);
		} else {
			fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
		}
	} else {
		fderr = dup(tmperr);
	}
	dup2(fderr, 2);
	close(fderr);

	// For every simple command fork a new process
    int ret;
    for (int i = 0; i < _numOfSimpleCommands; i++) {
		//redirect input
		dup2(fdin, 0);
		close(fdin);
		//setup output
		int fdout = tmpout;
		if (i == _numOfSimpleCommands-1) {
    		// Last simple command
    		if(_outFile) {
				if (_append) {
      				fdout = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0660);
				} else {
					fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
				}
    		} else {
      			// Use default output
      			fdout = dup(tmpout);
			}
		} else {
			// Not last simple command
			//create pipe
			int fdpipe[2];
  			pipe(fdpipe);
  			fdout=fdpipe[1];
  			fdin=fdpipe[0];
 		}// if/else
		//Redirect output
		dup2(fdout, 1);
		close(fdout);
		char * cmd = _simpleCommands[i]->_arguments[0];
		if (strcmp(cmd, "setenv") == 0) {
			//setenv
			setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);
		} else if (strcmp(cmd, "unsetenv") == 0) {
			//unsetenv
			unsetenv(_simpleCommands[i]->_arguments[1]);
		} else if (strcmp(cmd, "cd") == 0) {
			//cd
			if (_simpleCommands[i]->_arguments[1] == NULL)
				chdir(getenv("HOME"));
			else {
				int r = chdir(_simpleCommands[i]->_arguments[1]);
				if (r == -1)
					perror("No such file or directory");
			} 
		} else {
			ret = fork();
			//Create child process
			if (ret == 0) {
				//child
      			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
      			perror("execvp");
				_exit(1);
			} else if (ret < 0) {
				perror("fork");
				return;
			}
			_pid = ret;
		}
		setenv("_", strdup(_simpleCommands[i]->_arguments[_simpleCommands[i]->_numOfArguments - 1]), 1);
		// Parent shell continue
	} //for

	//restore in/out defaults
	dup2(tmpin,0);
	dup2(tmpout,1);
	dup2(tmperr, 2);
	close(tmpin);
	close(tmpout);
	close(tmperr);
	
	if (!_background) {
 		// wait for last process
  		waitpid(ret, NULL, 0);
		char * lastPID = (char *) malloc(sizeof(char) * 8);
		sprintf(lastPID, "%d", ret);
		setenv("?", lastPID, 1);
	} else {
		backgroundProcs[backgroundIndex++] = ret;
		char * lastPID = (char *) malloc(sizeof(char) * 8);
		sprintf(lastPID, "%d", ret);
		setenv("!", lastPID, 1);
	}

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}


void Command::prompt() {
	if (isatty(0)) {
		char * p = getenv("PROMPT");
		if (p)
			printf(CYN "%s" RESET, p);
		else
			printf(CYN "myshell>" RESET);
	}
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

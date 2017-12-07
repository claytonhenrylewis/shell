#ifndef command_h
#define command_h

#include "simpleCommand.hh"
#include <stdlib.h>

// Command Data Structure

struct Command {
  int _numOfAvailableSimpleCommands;
  int _numOfSimpleCommands;
  SimpleCommand ** _simpleCommands;
  char * _outFile;
  char * _inFile;
  char * _errFile;
  int _background;
  int _append;
  int _appendErr;
  int _pid;

  void prompt();
  void print();
  void execute();
  void clear();
  void exitShell();
  void exitCommand();

  int * backgroundProcs;
  int backgroundIndex;

  int isBackgroundProc(int pid);
  void removeBackgroundProc(int pid);

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );
  void insertOutput( char * out );
  void subShell( char * command );

  static Command _currentCommand;
  static SimpleCommand *_currentSimpleCommand;
};


/*{
	for (int i = 0; i <= backgroundIndex; i++) {
		if (backgroundProcs[i] == pid)
			return true;
	}
	return false;
}*/

#endif

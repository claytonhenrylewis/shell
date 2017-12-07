#include <string.h>

#ifndef simplcommand_h
#define simplecommand_h

struct SimpleCommand {
  // Available space for arguments currently preallocated
  int _numOfAvailableArguments;

  // Number of arguments
  int _numOfArguments;
  char ** _arguments;

  SimpleCommand();
  void insertArgument( char * argument );
  void subShell( char * command );
  void insertEnv( char * argument );
  void sort(int i, int j);
};

static int cmpstr(const void* a, const void* b) { 
    const char** aa = (const char**)a;
    const char** bb = (const char**)b;
	int s = strcmp(*aa, *bb);
    return strcmp(*aa, *bb);
}

#endif


/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <string_val> WORD SUB ENV
%token NOTOKEN GREAT NEWLINE GREATGREAT PIPE AMPERSAND LESS GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT EXIT SOURCE

%{
//#define yylex yylex
#include <sys/types.h>
#include <cstdio>
#include "command.hh"
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>

#define MAXFILENAME 1024

void yyerror(const char * s);
int yylex();
void yyrestart (FILE *input_file  );

/*int cmpstr(const void* a, const void* b) { 
    const char* aa = (const char*)a;
    const char* bb = (const char*)b;
    return strcmp(aa, bb);
}*/

void expandWildcard(char * prefix, char * suffix) {
	if ((strlen(prefix) > 0) && (prefix[0] == '~')) {
		prefix++;
		char * nP = (char *) malloc(sizeof(char) * MAXFILENAME);
		if (prefix[0] == '/')
			nP = getenv("HOME");
		else
			sprintf(nP, "/homes/");
		nP = strcat(nP, prefix);
		expandWildcard(nP, suffix);
		return;
	}
	char * suffixOrig = strdup(suffix);
	//printf("EXPAND: %s %s \n", prefix, suffix);

	if (suffix[0]== 0) {
       // suffix is empty. Put prefix in argument.
	   if ((prefix[0] == '/') && (prefix[1] == '/'))
			prefix = prefix + 1;
       Command::_currentSimpleCommand->insertArgument(strdup(prefix));
       return;
	}
	// Obtain the next component in the suffix 
	// Also advance suffix.
	char * s = strchr(suffix, '/');
	char component[MAXFILENAME];
	if (s!=NULL){ // Copy up to the first “/”
		strncpy(component,suffix, s-suffix);
		suffix = s + 1; }
	else { // Last part of path. Copy whole thing.
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}
	char newPrefix[MAXFILENAME];

	if ((strchr(component, '*') == NULL) && (strchr(component, '?') == NULL)) {
		// component does not have wildcards
		if ((strlen(prefix) > 0) || (suffixOrig[0] == '/'))
			sprintf(newPrefix, "%s/%s", prefix, component);
		else
			sprintf(newPrefix, "%s", component);
		expandWildcard(newPrefix, suffix);
		return;
	}
	// 1. Convert component to regular expression
	// Convert “*” -> “.*”
	//		   “?” -> “.”
	//		   "." -> "\."
	//		   others
	// Add ^ and $ to match the beginning and the end of the word.
	// Allocate enough space for regular expression
	char * reg = (char*)malloc(2*strlen(component)+10); 
	char * a = component;
	char * r = reg;
	*r = '^'; r++; // match beginning of line
	while (*a) {
		if (*a == '*') { *r='.'; r++; *r='*'; r++; }
		else if (*a == '?') { *r='.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;} else { *r=*a; r++;}
		a++;
	}
	*r='$'; r++; *r=0; // match end of line and add null char

	// 2. compile regular expression. See lab3-src/regular.cc
	regex_t re;
	int result = regcomp(&re, reg, REG_EXTENDED | REG_NOSUB);
	if (result != 0) {
		fprintf(stderr, "%s: Bad regular expression \"%s\"\n", component, reg);
		exit(-1);
	}

	// 3. List directory and add as arguments the entries // that match the regular expression
	char * d;
	if (strlen(prefix) == 0)
		d = ".";
	else 
		d = prefix;
	DIR * dir = opendir(d);
	if (dir == NULL) {
		//perror("opendir");
		return;
	}
	struct dirent * ent;
	int maxEntries = 20;
	int nEntries = 0;
	//char ** array = (char**) malloc(maxEntries*sizeof(char*));
	while ( (ent = readdir(dir)) != NULL) {
		// Check if name matches
		regmatch_t match;
		result = regexec( &re, ent->d_name, 1, &match, 0);
		if ( result == 0 ) {
			/*if (nEntries == maxEntries) {
				maxEntries *=2;
				array = (char **) realloc(array, maxEntries*sizeof(char*));
				assert(array!=NULL);
 			}*/
  			if (ent->d_name[0] == '.') {
    			if (component[0] == '.') {
      				/*array[nEntries]= strdup(ent->d_name);
					nEntries++;*/
					if ((strlen(prefix) > 0) || (suffixOrig[0] == '/'))
						sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
					else
						sprintf(newPrefix,"%s", ent->d_name);
					expandWildcard(newPrefix,suffix);
				}
			}
  			else {
				/*array[nEntries]= strdup(ent->d_name);
				nEntries++;*/
				if ((strlen(prefix) > 0) || (suffixOrig[0] == '/'))
					sprintf(newPrefix,"%s/%s", prefix, ent->d_name);
				else
					sprintf(newPrefix,"%s", ent->d_name);
				expandWildcard(newPrefix,suffix);
			}
		}
	}
	closedir(dir);
 	//sortArrayStrings(array, nEntries);
	//qsort(array, nEntries, sizeof(char *), cmpstr);
	// Add arguments
	/*for (int i = 0; i < nEntries; i++) {
    	Command::_currentSimpleCommand->insertArgument(array[i]);
	}
	free(array);*/
	regfree(&re);
}

%}

%%

goal:
  commands
  ;

commands:
  | command
  | commands command
  ;

command:
  simple_command
  ;


simple_command:	
  /*END_OF_FILE {
    yyrestart(stdin);
  }
  |*/ EXIT NEWLINE{
    Command::_currentCommand.exitShell();
  }
  | SOURCE WORD NEWLINE {
    FILE * fp;
	fp = fopen($2, "r");
    yyrestart(fp);
	yyparse();
	yyrestart(stdin);
  }
  | pipe_list iomodifier_list background_opt NEWLINE{
    /*printf("   Yacc: Execute command\n");*/
    Command::_currentCommand.execute();
  }
  | NEWLINE 
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Command::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  SUB {
    Command::_currentSimpleCommand->subShell( $1 );
  }
  | ENV {
	Command::_currentSimpleCommand->insertEnv( $1 );
  }
  | WORD {
    /*printf("   Yacc: insert argument \"%s\"\n", $1);*/
    /*Command::_currentSimpleCommand->insertArgument( $1 );*/
	int i = Command::_currentSimpleCommand->_numOfArguments;
	expandWildcard( "", $1 );
	int j = Command::_currentSimpleCommand->_numOfArguments;
	Command::_currentSimpleCommand->sort(i, j);
	if (i == j)
		Command::_currentSimpleCommand->insertArgument( $1 );
  } 
  ;

command_word:
  WORD {
    /*printf("   Yacc: insert command \"%s\"\n", $1);*/
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

iomodifier_opt:
  GREATGREAT WORD {
    /*printf("   Yacc: insert output \"%s\"\n", $2);*/
    Command::_currentCommand.insertOutput( $2 );
    /*printf("   Yacc: append\n");*/
    Command::_currentCommand._append = 1;
  }
  | GREAT WORD {
    /*printf("   Yacc: insert output \"%s\"\n", $2);*/
    Command::_currentCommand.insertOutput( $2 ) ;
    Command::_currentCommand._append = 0;
}
  | GREATGREATAMPERSAND WORD {
    /*printf("   Yacc: insert output \"%s\"\n", $2);*/
    Command::_currentCommand.insertOutput( $2 );
    /*printf("   Yacc: insert stderr output \"%s\"\n", $2);*/
    Command::_currentCommand._errFile = $2;
    /*printf("   Yacc: append\n");*/
    Command::_currentCommand._append = 1;
	Command::_currentCommand._appendErr = 1;
  }
  | GREATAMPERSAND WORD {
    /*printf("   Yacc: insert output \"%s\"\n", $2);*/
    Command::_currentCommand.insertOutput( $2 );
    /*printf("   Yacc: insert stderr output \"%s\"\n", $2);*/
    Command::_currentCommand._errFile = $2;
    Command::_currentCommand._append = 0;
  }
  | TWOGREAT WORD {
    /*printf("   Yacc: insert stderr output \"%s\"\n", $2);*/
    Command::_currentCommand._errFile = $2;
  }
  | LESS WORD {
    /*printf("   Yacc: insert input \"%s\"\n", $2);*/
    Command::_currentCommand._inFile = $2;
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  | /*can be empty*/
  ;

background_opt:
  AMPERSAND {
    /*printf("   Yacc: background\n");*/
    Command::_currentCommand._background = 1;
  }
  | /* empty */
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif

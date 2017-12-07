
/*
 *
 * CS-252 Fall 2017
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include "y.tab.hh"
#include <unistd.h>

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.

  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%%

"exit" {
  return EXIT;
}

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" { 
  return GREATGREAT; 
}

"|" { 
  return PIPE;
}

"&" { 
  return AMPERSAND;
}

"<" {
  return LESS;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

"source" {
  return SOURCE;
}

`[^\t\n][^\t\n]*` {
  char * p = yytext;
  p[strlen(p)-1] = 0;
  p++;
  yylval.string_val = strdup(p);
  return SUB;
}

\$\{[^ \t\n][^ \t\n]*\} {
  char * p = yytext;
  p[strlen(p)-1] = 0;
  p += 2;
  yylval.string_val = strdup(p);
  return ENV;
}

[^ \t\n][^ \t\n]*  {
  /* Assume that file names have only alpha chars */
  //Handle escape chars
  char *pr = yytext;
  char *pw = yytext;
  int slashCount = 0;
  while (*pr) {
    *pw = *pr++;
    if (*pw == '\\' ) {
      slashCount++;
      if ((slashCount % 2) == 0)
		pw++;
    } else {
      pw++;
      slashCount = 0;
    }
  }
  *pw = 0;
  yylval.string_val = strdup(yytext);
  return WORD;
}

\"[^\t\n][^\t\n]*\" {
  char * p = yytext;
  p++;
  p[strlen(p)-1] = 0;
  char *pr = p;
  char *pw = p;
  while (*pr) {
    *pw = *pr++;
    pw += (*pw != '\\' );
  }
  *pw = 0;
  yylval.string_val = strdup(p);
  return WORD;
}



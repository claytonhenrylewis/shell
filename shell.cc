#include "command.hh"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

//int * backgroundProcs = (int *) malloc(sizeof(int) * 10);
//int backgroundIndex = 0;
int yyparse(void);
void yyrestart (FILE *input_file  );

extern "C" void handle( int sig )
{
	if (Command::_currentCommand._pid != -1) {
		kill(Command::_currentCommand._pid, sig);
	} else {
		printf("\n");
		Command::_currentCommand.prompt();
	}
}

void zombie(int sig, siginfo_t *info, void *ucontext) {
	int status;
	int pid = info->si_pid;
	wait3(&status,0,NULL);
	if (Command::_currentCommand.isBackgroundProc(pid)) {
		Command::_currentCommand.removeBackgroundProc(pid);
		printf("\n%d exited\n", pid);
		Command::_currentCommand.prompt();
	}
	for (int i = 0; i < Command::_currentCommand.backgroundIndex; i++) {
		pid = Command::_currentCommand.backgroundProcs[i];
		if (pid != -1) {
			wait4(pid,&status,0,NULL);
			Command::_currentCommand.removeBackgroundProc(pid);
			printf("\n%d exited\n", pid);
			Command::_currentCommand.prompt();
		}
	} 
}

int main() {
	Command::_currentCommand.prompt();

	struct sigaction sa;
    sa.sa_handler = handle;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

	struct sigaction sa1;
	sa1.sa_sigaction = zombie;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_flags = SA_RESTART | SA_SIGINFO;

	if(sigaction(SIGINT, &sa, NULL)){
        perror("sigaction");
        exit(-1);
    }

	if (sigaction(SIGCHLD, &sa1, NULL)){
		perror("sigaction");
		exit(-1);
	}

	/*FILE * fp = fopen(".shellrc", "r");
	yyrestart(fp);
	yyparse();
	yyrestart(stdin);*/
	yyparse();
}

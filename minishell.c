#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>


#define NV 20 /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */
/*
shell prompt
*/
void prompt(void) {
    fprintf(stdout, "\n msh> ");
    fflush(stdout); // allows u to see prompt immediately
}

//signal handler
void sigchld_handler(int sig){
    int olderrno = errno;
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("\n[Background] PID %d finished\n", pid);
    }
    errno = olderrno;
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal; /* value returned by fork sys call */
    //int wpid; /* value returned by wait */
    char *v[NV]; /* array of pointers to command line tokens
    */
    char *sep = " \t\n"; /* command line token separators */
    int i; /* parse index */
    
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Restart interrupted sys calls, ignore stopped children

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    /* prompt for and process one command line at a time */
    while (1) { /* do Forever */
        prompt();
        fgets(line, NL, stdin); //waits for u to type a line and press enter 
        fflush(stdin);
        if (feof(stdin)) { /* non-zero on EOF */
            fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
            feof(stdin), ferror(stdin));
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000'){
            continue; /* to prompt */
        }
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep); //replaces separators with '\0'
            if (v[i] == NULL){
                break;
            }
        }
        int bg = 0;
        if(strcmp(v[i-1], "&") == 0){
            bg = 1;
            v[i-1] = NULL;
        }
/* assert i is number of tokens + 1 */
/* fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
            case -1: /* fork returns error to parent process */
            {
            break;
            }
            case 0: /* code executed only by child process */
            {
            execvp(v[0], v);
            perror("execvp");
            }
            default: /* code executed only by parent process */
            {
            
            if(bg){
                printf("[Background] %s started with PID %d\n", v[0], frkRtnVal);
            }else{
                wait(0);
                printf("%s done \n", v[0]);
            }

            break;
            }
        } /* switch */
    } /* while */
}
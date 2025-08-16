#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20  /* max number of command tokens */
#define NL 100 /* input buffer size */

char line[NL];       
int job_number = 1;   

char last_cmdline[NL]; 
int last_job_num = 0;  

void prompt(void) {
    fflush(stdout); // ensures prompt is shown immediately
}

void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Print only if it was a background job
        if (last_job_num > 0) {
            printf("\n[%d]+ Done %s\n", last_job_num, last_cmdline);
            fflush(stdout);
            last_job_num = 0; // reset
        }
    }
    errno = olderrno;
}

int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal;
    char *v[NV];
    char *sep = " \t\n";
    int i;

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
        prompt();
        if (!fgets(line, NL, stdin))
            break;
        fflush(stdin);

        if (feof(stdin)) {
            //fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),feof(stdin), ferror(stdin));
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
            continue;
        }

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL) break;
        }

        int bg = 0;
        for (int j = 0; j < i; j++) {
            if (v[j] && strcmp(v[j], "&") == 0) {
                bg = 1;
                v[j] = NULL; // remove &
                break;
            }
        }

        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(v[1]) == -1) {
                perror("chdir");
            }
            continue;
        }

        switch (frkRtnVal = fork()) {
            case -1:
                perror("fork");
                break;
            case 0: // child
                execvp(v[0], v);
                perror("execvp");
                exit(1);
            default: // parent
                if (bg) {
                    last_job_num = job_number++;
                    last_cmdline[0] = '\0';
                    for (int k = 0; v[k] != NULL; k++) {
                        strcat(last_cmdline, v[k]);
                        if (v[k+1] != NULL) strcat(last_cmdline, " ");
                    }
                    //printf("[%d]%d\n", last_job_num, frkRtnVal);
                    //fflush(stdout);
                } else {
                    waitpid(frkRtnVal, NULL, 0);
                    //printf("%s done \n", v[0]);
                }
                break;
        }
    }
}

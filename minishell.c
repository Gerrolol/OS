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
char line[NL]; /* command input buffer */

/* shell prompt */
void prompt(void) {
    fprintf(stdout, "\n msh> ");
    fflush(stdout); // ensures immediate prompt
}

/* signal handler for background processes */
void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Background process %d finished\n", pid);
        fflush(stdout);
    }
    errno = olderrno;
}

int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal; /* fork return value */
    char *v[NV];   /* array of command line tokens */
    char *sep = " \t\n"; /* token separators */
    int i;

    /* setup SIGCHLD handler */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    /* shell loop */
    while (1) {
        prompt();

        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                exit(0);
            } else {
                perror("fgets");
                continue;
            }
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
            continue;

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL) break;
        }

        /* detect background & remove it from args */
        int bg = 0;
        for (int j = 0; j < i; j++) {
            if (v[j] && strcmp(v[j], "&") == 0) {
                bg = 1;
                v[j] = NULL;
                break;
            }
        }

        /* built-in cd */
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                char *home = getenv("HOME");
                if (home == NULL) {
                    fprintf(stderr, "cd: HOME not set\n");
                } else if (chdir(home) == -1) {
                    perror("chdir");
                }
            } else {
                if (chdir(v[1]) == -1) {
                    perror("chdir");
                }
            }
            continue;
        }

        /* fork and exec */
        frkRtnVal = fork();
        if (frkRtnVal == -1) {
            perror("fork");
            continue;
        }

        if (frkRtnVal == 0) { /* child */
            execvp(v[0], v);
            perror("execvp"); /* only reached if exec fails */
            exit(1);
        } else { /* parent */
            if (bg) {
                printf("Background process %d started\n", frkRtnVal);
                fflush(stdout);
            } else {
                if (waitpid(frkRtnVal, NULL, 0) == -1) {
                    perror("waitpid");
                } else {
                    printf("%s done\n", v[0]);
                    fflush(stdout);
                }
            }
        }
    }
}

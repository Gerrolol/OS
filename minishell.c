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
#define MAXJOBS 100

struct job {
    int job_num;          /* job number */
    pid_t pid;            /* process ID */
    char cmdline[NL];     /* command line */
};

char line[NL];
struct job jobs[MAXJOBS];
int job_number = 1;

// void prompt(void) {
//     //fflush(stdout); // ensures prompt is shown immediately
// }

void remove_job(pid_t pid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].pid = 0;
            jobs[i].job_num = 0;
            jobs[i].cmdline[0] = '\0';
            break;
        }
    }
}

void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAXJOBS; i++) {
            if (jobs[i].pid == pid) {
                // Print finish message in bash format
                printf("[%d]+ Done %s\n", jobs[i].job_num, jobs[i].cmdline);
                fflush(stdout);
                remove_job(pid);
                break;
            }
        }
    }
    errno = olderrno;
}

int add_job(pid_t pid, const char *cmdline) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].job_num = job_number++;
            strncpy(jobs[i].cmdline, cmdline, NL-1);
            jobs[i].cmdline[NL-1] = '\0';
            return jobs[i].job_num;
        }
    }
    return -1; // No space
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

    memset(jobs, 0, sizeof(jobs));

    while (1) {
        //prompt();
        if (!fgets(line, NL, stdin))
            break;
        fflush(stdin);

        if (feof(stdin)) {
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
                //fprintf(stderr, "cd: missing argument\n");
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
                    // Copy the original input line
                    char rawline[NL];
                    strncpy(rawline, line, NL-1);
                    rawline[NL-1] = '\0';

                    // Remove trailing newline
                    size_t len = strlen(rawline);
                    if (len > 0 && rawline[len-1] == '\n') rawline[len-1] = '\0';

                    // Remove trailing '&' and spaces
                    char *amp = strrchr(rawline, '&');
                    if (amp) {
                        *amp = '\0';
                        // trim spaces before '&'
                        while (amp > rawline && (*(amp-1) == ' ' || *(amp-1) == '\t')) {
                            *(--amp) = '\0';
                        }
                    }

                    // Add the job to the job list
                    int jobnum = add_job(frkRtnVal, rawline);
                    if (jobnum > 0) {
                        printf("[%d] %d\n", jobnum, frkRtnVal);
                        fflush(stdout);
                    } else {
                        //printf("Job list full!\n");
                    }
                }
        }
    }
}

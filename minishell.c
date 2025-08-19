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

//globals
char line[NL];
struct job jobs[MAXJOBS]; //keep track of all background jobs
int job_number = 1;

// void prompt(void) {
//     //fflush(stdout); // ensures prompt is shown immediately
// }

// Remove a job from the jobs array by its PID  
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

// Signal handler for SIGCHLD to handle child process termination
void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAXJOBS; i++) {
            if (jobs[i].pid == pid) {
                printf("[%d]+ Done\t\t\t %s\n", jobs[i].job_num, jobs[i].cmdline);
                fflush(stdout);
                remove_job(pid);
            }
        }
    }
    errno = olderrno;
}

// void sigchld_handler(int sig) {
//     int olderrno = errno;
//     pid_t pid;
//     int status;
//     while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
//         printf("SIGCHLD fired for pid %d\n", pid);
//         fflush(stdout);
//     }
//     errno = olderrno;
// }

// Add a job to the jobs array
// Returns the job number if successful, or -1 if no spac   e is available
// The job number is incremented each time a new job is added
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

    // signal handler setup
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
        if (!fgets(line, NL, stdin)) {
            break;   // EOF on stdin
        }
        if (feof(stdin)) {
            break;
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
                v[j] = NULL;
                break;
            }
        }

        if (strcmp(v[0], "cd") == 0) {
            if (v[1] != NULL && chdir(v[1]) == -1) {
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
                    char cmdline[NL] = "";
                    for (int k = 0; v[k] != NULL; k++) {
                        strcat(cmdline, v[k]);
                        if (v[k+1] != NULL) strcat(cmdline, " ");
                    }
                    int jobnum = add_job(frkRtnVal, cmdline);
                    if (jobnum > 0) {
                        printf("[%d] %d\n", jobnum, frkRtnVal);
                        fflush(stdout);
                    }
                } else {
                    waitpid(frkRtnVal, NULL, 0);
                }
                break;
        }
    }

    // *** NEW PART: after EOF, wait until all background jobs finish ***
    int alive;
    do {
        alive = 0;
        for (int i = 0; i < MAXJOBS; i++) {
            if (jobs[i].pid != 0) {
                alive = 1;
                break;
            }
        }
        if (alive) {
            pause();  // sleep until a signal (like SIGCHLD) wakes us
        }
    } while (alive);

    return 0;
}

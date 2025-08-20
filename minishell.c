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

void prompt(void) {
    fflush(stdout); // ensures prompt is shown immediately
}

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
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAXJOBS; i++) {
            if (jobs[i].pid == pid) {
                // Format like Bash
                printf("[%d]+ Done                 %s\n",jobs[i].job_num, jobs[i].cmdline);
                fflush(stdout);
                remove_job(pid);
            }
        }
    }
}


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
    signal(SIGCHLD, sigchld_handler);


    while (1) {
        prompt();
        if (!fgets(line, NL, stdin)) {
            break;   // EOF on stdin
        }
        if (feof(stdin)) {
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
            continue;
        }
        
        char commandLine[NL];
        strncpy(commandLine, line, NL-1);
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
                if(execvp(v[0], v) == -1){
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            default: // parent
                if (bg) {
                    commandLine[strcspn(commandLine, "&")] = '\0';  // remove &
                    int jobnum = add_job(frkRtnVal, commandLine);
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

    //after EOF, wait until all background jobs finish ***
    // int alive;
    // do {
    //     alive = 0;
    //     for (int i = 0; i < MAXJOBS; i++) {
    //         if (jobs[i].pid != 0) {
    //             alive = 1;
    //             break;
    //         }
    //     }
    //     if (alive) {
    //         pause();  // sleep until a signal (like SIGCHLD) wakes us
    //     }
    // } while (alive);
}

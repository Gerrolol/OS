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
#define MAXJOBS 100 //just set the size of the array to hold jobs

// added a struct to hold job information as the original code had no concept of background jobs.
//having the struct means I can keep track of multiple background jobs and their status
struct job { 
    int job_num;          /* job number */
    pid_t pid;            /* process ID */
    char cmdline[NL];     /* command line */
};

//globals
char line[NL];
static struct job jobs[MAXJOBS]; //this array of job structs is needed as you need to maintain a list of the active background jobs
static int job_number = 1; // job number starts at 1 and increments for each new job

void prompt(void) {
    fflush(stdout); //removed print statemtn
}

// I implemented a remove_job function to remove a job from the jobs array after it has completed
// It searches for the job by its PID and resets its fields
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

// Signal handler for SIGCHLD to handle child process termination. I used a signal handler to catch the SIGCHLD signal 
// without this, completed jobs wouldn't print "Done" message
void sigchld_handler(int sig) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAXJOBS; i++) {
            if (jobs[i].pid == pid) {
                printf("[%d]+ Done                 %s\n",jobs[i].job_num, jobs[i].cmdline); //output for completed job
                remove_job(pid);
            }
        }
    }
}

// Add a job to the jobs array
// Returns the job number if successful, or -1 if no space is available
// the add job and remove jobs are neccessary to handle multiple background processes and clean up after they finis
int add_job(pid_t pid, const char *cmdline) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].job_num = job_number++;
            strcpy(jobs[i].cmdline, cmdline);
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
        fgets(line, NL, stdin);
      
        if (feof(stdin)) {
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
            continue;
        }
        // Create a copy of the command line for job tracking
        // This is necessary because strtok modifies the string it processes
        // and we need to keep the original command line intact for job tracking
        char commandLine[NL];
        strncpy(commandLine, line, NL-1);
        commandLine[strcspn(commandLine, "\n")] = '\0'; // <-- strip newline

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);  
            if (v[i] == NULL) break;
        }

        //here I check for background jobs by looking for the '&' character
        // If found, I set the bg flag to 1 and replace '&' with NULL
        // I added this so that it can differentiate between foreground and background commands
        int bg = 0;
        for (int j = 0; j < i; j++) {
            if (v[j] && strcmp(v[j], "&") == 0) {
                bg = 1;
                v[j] = NULL;
                break;
            }
        }
        // If the command is "cd", I handle it separately
        // I check if the first token is "cd" and if so, I change the
        // current working directory using chdir
        // If chdir fails, I print an error message using perror
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] != NULL && chdir(v[1]) == -1) {
                perror("chdir");
            }
            continue;
        }

        switch (frkRtnVal = fork()) {
            case -1: //fork failed, I added a perror statement to print the error message
                perror("fork");
                break;
            case 0: // child
                if(execvp(v[0], v) == -1){ // execvp failed, I added a perror statement to print the error message
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            default: // parent
            //instead of wait(0) for background jobs, I used waitpid with WNOHANG to avoid blocking
            // the if statement checks if the bg flag is set as done so earlier
                if (bg) { 
                    commandLine[strcspn(commandLine, "&")] = '\0';  // remove &
                    int jobnum = add_job(frkRtnVal, commandLine); // add job to the jobs array
                    if (jobnum > 0) { // if job was added successfully print job number and return value as required 
                        printf("[%d] %d\n", jobnum, frkRtnVal);
                        fflush(stdout);
                    }
                } else { // foreground job
                    if(wait(NULL) == -1) {
                        perror("wait");
                    }
                }
                break;
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
/*
Write a C program called even.c that takes the input parameter n from the command line and prints the first “n” even numbers (zero is even). 
We want the program to run slowly, so that it executes long enough to receive a signal. 
To achieve this, you should place a sleep(5) after every print statement.  
Compile and run to test it works ok.

*/
void sighup(){
    printf("Ouch!\n");
}

void sighint(){
    printf("Yeah!\n");
}

int main(int argc, char *argv[]){   
    signal(SIGHUP, sighup);
    signal(SIGINT, sighint);
    int n = atoi(argv[1]);

    for(int i=0; i<n; i++){
        if(i % 2 == 0){
            printf("%d\n", i);
            sleep(5);
        }
    }

    return  0;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

struct msg {
    long int t;
    char x[6];
};

int main() {
    int m;
    pid_t p1, p2;
    struct msg m1;
    char w[20];
    
    m = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    
    printf("Please enter the workspace name:\n");
    scanf("%s", w);
    
    if (strcmp(w, "cse321") != 0) {
        printf("Invalid workspace name\n");
        exit(1);
    }
    
    m1.t = 1;
    strcpy(m1.x, w);
    msgsnd(m, &m1, sizeof(m1.x), 0);
    printf("Workspace name sent to otp generator from log in: %s\n", m1.x);
    
    p1 = fork();
    if (p1 == 0) { 
        msgrcv(m, &m1, sizeof(m1.x), 1, 0);
        printf("OTP generator received workspace name from log in: %s\n", m1.x);
        
        sprintf(m1.x, "%d", getpid());
        m1.t = 2;
        msgsnd(m, &m1, sizeof(m1.x), 0);
        printf("OTP sent to log in from OTP generator: %s\n", m1.x);
        
        m1.t = 3;
        msgsnd(m, &m1, sizeof(m1.x), 0);
        printf("OTP sent to mail from OTP generator: %s\n", m1.x);
        
        p2 = fork();
        if (p2 == 0) {  
            msgrcv(m, &m1, sizeof(m1.x), 3, 0);
            printf("Mail received OTP from OTP generator: %s\n", m1.x);
            
            m1.t = 4;
            msgsnd(m, &m1, sizeof(m1.x), 0);
            printf("OTP sent to log in from mail: %s\n", m1.x);
            exit(0);
        }
        wait(NULL);
        exit(0);
    }
    
    wait(NULL);
    
    struct msg m2, m3;
    msgrcv(m, &m2, sizeof(m2.x), 2, 0);
    printf("Log in received OTP from OTP generator: %s\n", m2.x);
    
    msgrcv(m, &m3, sizeof(m3.x), 4, 0);
    printf("Log in received OTP from mail: %s\n", m3.x);
    
    if (strcmp(m2.x, m3.x) == 0) {
        printf("OTP Verified\n");
    } else {
        printf("OTP Incorrect\n");
    }
    
    msgctl(m, IPC_RMID, NULL);
    return 0;
}

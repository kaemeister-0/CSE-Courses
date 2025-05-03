#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

struct shared {
    char s[100];
    int b;
};

int main() {
    pid_t p;
    int f[2], m;
    key_t k = 1234;
    struct shared d;

    m = shmget(k, sizeof(struct shared), 0666 | IPC_CREAT);
    if (m == -1) {
        perror("shmget");
        exit(1);
    }

    d = *(struct shared *)shmat(m, NULL, 0);
    if (&d == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    if (pipe(f) == -1) {
        perror("pipe");
        exit(1);
    }

    d.b = 1000;

    printf("Provide Your Input From Given Options:\n");
    printf("1. Type a to Add Money\n");
    printf("2. Type w to Withdraw Money\n");
    printf("3. Type c to Check Balance\n");
    
    fgets(d.s, sizeof(d.s), stdin);
    d.s[strcspn(d.s, "\n")] = 0;
    printf("Your selection: %s\n", d.s);

    p = fork();
    if (p < 0) {
        perror("fork");
        exit(1);
    }

    if (p == 0) {
        close(f[0]);
        
        if (strcmp(d.s, "a") == 0) {
            int a;
            printf("\nEnter amount to be added:\n");
            scanf("%d", &a);
            if (a > 0) {
                d.b += a;
                printf("Balance added successfully\n");
                printf("Updated balance after addition:\n%d\n", d.b);
            } else {
                printf("Adding failed, Invalid amount\n");
            }
        } else if (strcmp(d.s, "w") == 0) {
            int a;
            printf("\nEnter amount to be withdrawn:\n");
            scanf("%d", &a);
            if (a > 0 && a <= d.b) {
                d.b -= a;
                printf("Balance withdrawn successfully\n");
                printf("Updated balance after withdrawal:\n%d\n", d.b);
            } else {
                printf("Withdrawal failed, Invalid amount\n");
            }
        } else if (strcmp(d.s, "c") == 0) {
            printf("\nYour current balance is:\n%d\n", d.b);
        } else {
            printf("\nInvalid selection\n");
        }

        write(f[1], "Thank you for using\n", 20);
        shmdt(&d);
        exit(0);
    } else {
        close(f[1]);
        wait(NULL);
        
        char msg[100];
        read(f[0], msg, sizeof(msg));
        printf("%s", msg);
        
        shmdt(&d);
        shmctl(m, IPC_RMID, NULL);
    }
    return 0;
}

#include <stdlib.h>

/**
 * getpid and getppid return two different values
 * https://stackoverflow.com/questions/15183427/getpid-and-getppid-return-two-different-values
 */
void is_parent(int id) {
    if (id == 0)
    {
        printf("Child : Hello I am the child process\n");
        printf("Child : Child’s PID: %d\n", getpid());
        printf("Child : Parent’s PID: %d\n\n", getppid());
    }
    else
    {
        printf("Parent : Hello I am the parent process\n");
        printf("Parent : Parent’s PID: %d\n", getpid());
        printf("Parent : Child’s PID: %d\n\n", id);
    }
}

int main() {
    int id;
    id = fork();
    printf("fork id value : %d\n", id);
    is_parent(id);
    
    id = fork();
    printf("fork id value : %d\n", id);
    is_parent(id);
    
    return 0;
}

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int i;
    int n = 24;
    int fd = open("./test.txt", O_CREAT | O_TRUNC | O_RDWR, 0600);
    char buf[n];
    struct flock lock;

    if (fd == -1)
    {
        perror("open");
        exit(1);
    }
    for (i = 0; i < 10; i++)
    {
        buf[2 * i] = 49 + i;
        buf[2 * i + 1] = '\n';
    }
    buf[i] = '\0';

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);

    if (write(fd, &buf, sizeof(buf)) != sizeof(buf))
    {
        perror("write");
        exit(1);
    }

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);

    if (close(fd) == -1)
    {
        perror("close");
        exit(1);
    }
    return 0;
}
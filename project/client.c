#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define BUF_SIZE 256

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j, efd;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];
    struct epoll_event event;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s host port ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    // efd = epoll_create1(0);
    // if (efd == -1)
    // {
    //     perror("epoll_create");
    //     abort();
    // }

    // event.data.fd = sfd;
    // event.events = EPOLLOUT | EPOLLET;
    // s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    // if (s == -1)
    // {
    //     perror("epoll_ctl");
    //     abort();
    // }

    s = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */

        if (close(sfd) == -1)
        {
            perror("close");
            exit(1);
        }
    }

    if (rp == NULL)
    { /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); /* No longer needed */

    /* Send remaining command-line arguments as separate
       datagrams, and read responses from server */

    // char hello_msg[] = "Hello";
    // len = strlen(hello_msg) + 1;
    // if (write(sfd, hello_msg, len) != len)
    // {
    //     fprintf(stderr, "partial/failed write\n");
    //     exit(EXIT_FAILURE);
    // }

    int n = 3;
    for (j = 0; j < n; j++)
    {
        char string_buf[20];

        fgets(string_buf, 20, stdin);
        len = strlen(string_buf) + 1;
        /* +1 for terminating null byte */

        if (len + 1 > BUF_SIZE)
        {
            fprintf(stderr,
                    "Ignoring long message in argument %d\n", j);
            continue;
        }

        if (write(sfd, string_buf, len) != len)
        {
            fprintf(stderr, "partial/failed write\n");
            exit(EXIT_FAILURE);
        }
        printf("Message sent.\n");

        nread = read(sfd, buf, BUF_SIZE);
        if (nread == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("Received %ld bytes: %s\n", (long)nread, buf);
    }

    if (close(sfd) == -1)
    {
        perror("close");
        exit(1);
    }

    exit(EXIT_SUCCESS);
}
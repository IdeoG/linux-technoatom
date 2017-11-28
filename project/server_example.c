/**
 *
 * Заметки по реализации epoll server:
 * 
 */

/** int epoll_create(int size);
 * Создает новый экзамепляр epoll
 * 
 * @params
 * size - устараевший параметр, но должен быть больше нуля
 * @return
 * возвращает файловый дескрптор для нового экземпляра epoll
 * 
 * http://man7.org/linux/man-pages/man2/epoll_create.2.html
 */

/** int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
 * Управление экземпляром epoll 
 * 
 * @params
 * epfd - epoll fd
 * op - operation by target (
 *  EPOLL_CTL_ADD - register new target for epoll instance
 *  EPOLL_CTL_MOD - changer the event associated with the target file
 *  EPOLL_CTL_DEL - deregister(remove) target 
 * )
 * fd - target fd
 * epoll_events:
 *  EPOLLIN - target fd доступ для операций чтения
 *  EPOLLOUT - target fd доступен для операций записи
 *  EPOLLPRI - условия fd для ошибок
 *  EPOLLET - устанавливает Edge Triggered поведение для fd
 *  EPOLLONESHOT - one-shot повдение для fd, т.е. он будет удален после одного обмена обмена данных после epoll_wait
 * struct epoll_event {
 *  uint32_t events;
 *  epoll_data_t data;
 * }
 * 
 * typedef union epoll_data {
 *  void *ptr;
 *  int  fd;
 *  uint32_t    u32;
 *  uint64_t    u64;
 * }
 * http://man7.org/linux/man-pages/man2/epoll_ctl.2.html
 */

/** int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
 * Wait for I/O event on an epoll fd instance
 * 
 * @params
 * timeout - блокировка epoll fd в мс. Он будет заблокирован, если:
 *  fd отправляет event
 *  вызов прерван обработчиком (signal_handler)
 *  timeout заврешен
 * 
 * @return
 * Возвращает число target fd, готовых для I/O операций или 0, если таковых нет
 * -1 при ошибке
 * events - возращает в ивенты fd, если они доступны
 * 
 */

// TODO: Sent message back to clients

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

#define MAXEVENTS 64

static int create_and_bind(char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0)
        {
            break;
        }

        close(sfd);
    }
    if (rp == NULL)
    {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(result);

    return sfd;
}

static int make_socket_non_blocking(int sfd)
{
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1)
    {
        perror("fcntl");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sfd = create_and_bind(argv[1]);
    if (sfd == -1)
        abort();

    s = make_socket_non_blocking(sfd);
    if (s == -1)
        abort();

    s = listen(sfd, SOMAXCONN);
    if (s == -1)
    {
        perror("listen");
        abort();
    }

    efd = epoll_create1(0);
    if (efd == -1)
    {
        perror("epoll_create");
        abort();
    }

    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1)
    {
        perror("epoll_ctl");
        abort();
    }

    events = calloc(MAXEVENTS, sizeof event);

    while (1)
    {
        int n, i;

        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
            {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            }

            else if (sfd == events[i].data.fd)
            {
                while (1)
                {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof in_addr;
                    infd = accept(sfd, &in_addr, &in_len);
                    if (infd == -1)
                    {
                        if ((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        else
                        {
                            perror("accept");
                            break;
                        }
                    }

                    s = getnameinfo(&in_addr, in_len,
                                    hbuf, sizeof hbuf,
                                    sbuf, sizeof sbuf,
                                    NI_NUMERICHOST | NI_NUMERICSERV);
                    if (s == 0)
                    {
                        printf("Accepted connection on descriptor %d "
                               "(host=%s, port=%s)\n",
                               infd, hbuf, sbuf);
                    }

                    s = make_socket_non_blocking(infd);
                    if (s == -1)
                        abort();

                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                    if (s == -1)
                    {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            }
            else
            {
                int done = 0;

                while (1)
                {
                    ssize_t count;
                    char buf[512];

                    count = read(events[i].data.fd, buf, sizeof buf);
                    if (count == -1)
                    {
                        if (errno != EAGAIN)
                        {
                            perror("read");
                            done = 1;
                        }
                        break;
                    }
                    else if (count == 0)
                    {
                        done = 1;
                        break;
                    }

                    s = write(1, buf, count);
                    if (s == -1)
                    {
                        perror("write");
                        abort();
                    }
                }

                if (done)
                {
                    printf("Closed connection on descriptor %d\n",
                           events[i].data.fd);

                    сlose(events[i].data.fd);
                }
            }
        }
    }

    free(events);

    close(sfd);

    return EXIT_SUCCESS;
}
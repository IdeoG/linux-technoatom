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

// https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/

/**
 *
 * TODO:
 * 1. Добавить циклобуфер для каждого клиента, чтобы быть уверенным в отправке данных.
 * 2. Для каждого клиента можно в контексте data_epol void *ptr, например указатель на struct циклобуфера
 * 3. Аллоцировать память с помощью malloc для циклобуфера в struct?
 * 4. Контекст (writable, id, buffer). Смогу правильно реализовать?
 * 5. writable flag в 0, если буффер(4096) заполнен и нам нужно отправить данные, после 
 * того мы сможем дальше принимать данные. Смогу разобраться?
 * 6. Когда приходит epollout? 
 * Нельзя было писать в сокет и вдруг можно писать!
 * 7. Как реализовать epollout обработчик? 
 * 8. Какой флаг выкатывается при закрытии сокета? Какую обработку повесить на этот неизвестный флаг?
 * 9. Как логгировать все данные в файл?
 * 10. Какой код лучше обернуть функциями? Send_all?
 * 11. Как проверить, можем мы передать данные клиенту или нет?
 * 12. Как написать клиента, который спит на 5 минут? Потеряются ли данные, которые к нему идут?
 * 13. Что делает syslog? Куда его вставить?
 * 14. Как посмотреть размер окна через TCP?
 * 15. Как добавить мультиплексирование данных с помощью poll?
 * 
 *
 */

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
#define BUF_SIZE 1024

struct client_content
{
    int is_writable;
    int fd;
    char ring[4096];
    int ring_init;
    int ring_end;
};

static int
make_socket_non_blocking(int sfd)
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

static int
create_and_bind(char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

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
            /* We managed to bind successfully! */
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

int main(int argc, char *argv[])
{
    int sfd, s;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    int sfd_storage_counter = 0;
    const int sfd_storage_max = 20;
    struct client_content *clients_storage[sfd_storage_max];
    int sfd_storage[sfd_storage_max];

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
    event.events = EPOLLIN | EPOLLET | EPOLLOUT;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1)
    {
        perror("epoll_ctl");
        abort();
    }

    /* Buffer where events are returned */
    events = calloc(MAXEVENTS, sizeof event);

    /* The event loop */
    while (1)
    {
        int n, i;

        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP))
            {
                /* Handle simple errors */

                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            }

            else if (sfd == events[i].data.fd)
            {
                /* New user is pending to be processed */

                if (sfd_storage_counter < sfd_storage_max)
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
                            if (errno == EAGAIN)
                                break;
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

                        /* Create struct for new user */

                        struct client_content *client;
                        client = malloc(sizeof(struct client_content));

                        client->fd = infd;
                        client->is_writable = 1;
                        client->ring_init = 0;
                        client->ring_end = 0;

                        event.data.ptr = client;
                        event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP;
                        s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1)
                        {
                            perror("epoll_ctl");
                            abort();
                        }
                        printf("New user has added.\n");
                        clients_storage[sfd_storage_counter] = client;
                        sfd_storage[sfd_storage_counter++] = infd;
                    }
                }
                else
                {
                    printf("Not enought memory for new fd\n");
                    for (int j = 0; j < sfd_storage_counter; j++)
                    {
                        printf("sfd_storage[%d]=%d\n", j, sfd_storage[j]);
                    }
                }
                continue;
            }
            else if (events[i].events & EPOLLIN)
            {
                /* The data is pending to be processed */

                int done = 0;

                printf("The data is pending to be processed. FD = %d\n", events[i].data.fd);
                while (1)
                {
                    ssize_t count;
                    char buf[BUF_SIZE] = {0};

                    count = read(events[i].data.fd, buf, sizeof buf);
                    if (count == -1)
                    {
                        fprintf(stdout, "Can't read from socket. Probably socket has closed.\n");
                        perror("read");
                        break;
                    }
                    else if (count == 0)
                    {
                        /* End of file. The remote has closed the
                         connection. */
                        done = 1;
                        break;
                    }

                    /* Write the buffer to standard output */
                    s = write(stdout, buf, count);
                    if (s == -1)
                    {
                        perror("write");
                        abort();
                    }

                    /* Send msg back to client */
                    int outfd = events[i].data.fd;

                    for (int k = 0; k < sfd_storage_counter; k++)
                    {
                        if (sfd_storage[k] == outfd)
                            continue;

                        /* Fill a ring buffer */

                        int ring_init = (clients_storage)[i]->ring_init;
                        int ring_end = (clients_storage)[i]->ring_end;
                        for (int j = 0; j < count; j++)
                        {
                            if (clients_storage[k]->is_writable == 1)
                            {
                                if (j + ring_init > 4096)
                                    clients_storage[k]->ring[ring_init - 4096] = buf[j];
                                else
                                    clients_storage[k]->ring[ring_init] = buf[j];
                                if ((ring_init + 1) == ring_end)
                                {
                                    clients_storage[k]->is_writable = 0;
                                    break;
                                }

                                ring_init++;
                                clients_storage[k]->ring_init++;
                            }
                        }
                        if (ring_init > 4096)
                            clients_storage[k]->ring_init -= 4096;

                        /* Write to socket */

                        s = write(sfd_storage[k], buf, count);
                        if (s == -1)
                        {
                            perror("write");
                            break;
                        }
                        else
                        {
                            if ((clients_storage[k]->ring_end + s) >= 4096)
                                clients_storage[k]->ring_end += s - 4096;
                            else
                                clients_storage[k]->ring_end += s;
                        }
                    }
                }

                if (done)
                {
                    printf("Closed connection on descriptor %d\n",
                           events[i].data.fd);

                    /* Closing the descriptor will make epoll remove it
                     from the set of descriptors which are monitored. */
                    close(events[i].data.fd);
                    for (int i = 0; i < sfd_storage_counter; i++)
                    {
                        if (events[i].data.fd == sfd_storage[i])
                        {
                            for (int j = i; j < sfd_storage_max - 1; j++)
                            {
                                sfd_storage[j] = sfd_storage[j + 1];
                                clients_storage[j] = clients_storage[j + 1];
                            }
                            continue;
                        }
                    }
                    sfd_storage_counter--;
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                int invalid_fd = 0;
                /* The socket is ready to be written for lost data */
                fprintf(stdout, "The socket is ready to be written for lost data.\n");
                // find socket.. костыль и велосипед..
                int j;
                for (j = 0; j < sfd_storage_counter; j++)
                {
                    if (clients_storage[j]->fd == events[i].data.fd)
                        break;
                    else
                    {
                        fprintf(stdout, "Invalid fd\n");
                        fprintf(stdout, "event fd: %d, storage_event %d\n", events[i].data.fd, clients_storage[j]->fd);
                        invalid_fd = 1;
                    }
                }
                if (invalid_fd == 1)
                    continue;
                fprintf(stdout, "Continue procsrring EPOLLOUT");
                int ring_init = clients_storage[j]->ring_init;
                int ring_end = clients_storage[j]->ring_end;
                if (ring_init < ring_end)
                {
                    int size = (4096 - ring_end) + ring_init;
                    char w_buf[size];
                    for (int k = 0; k < size; k++)
                    {
                        if ((ring_end + k) >= 4096)
                            w_buf[k] = clients_storage[j]->ring[ring_end + k - 4096];
                        else
                            w_buf[k] = clients_storage[j]->ring[ring_end + k];
                    }
                    /* Write to socket */

                    s = write(sfd_storage[j], w_buf, size);
                    if (s == -1)
                    {
                        perror("write");
                        break;
                    }
                    else
                    {
                        if ((clients_storage[j]->ring_end + s) >= 4096)
                            clients_storage[j]->ring_end += s - 4096;
                        else
                            clients_storage[j]->ring_end += s;
                    }
                }
                else
                {
                    int size = ring_init - ring_end;
                    char w_buf[size];
                    for (int k = 0; k < size; k++)
                    {
                        w_buf[k] = clients_storage[j]->ring[ring_end + k];
                    }

                    /* Write to socket */

                    s = write(sfd_storage[j], w_buf, size);
                    if (s == -1)
                    {
                        perror("write");
                        break;
                    }
                    else
                    {
                        if ((clients_storage[j]->ring_end + s) >= 4096)
                            clients_storage[j]->ring_end += s - 4096;
                        else
                            clients_storage[j]->ring_end += s;
                    }
                }
            }
        }
    }

    free(events);

    close(sfd);

    return EXIT_SUCCESS;
}
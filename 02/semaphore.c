#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sem.h>

/**
 * Немного документации к коду:
 * 
 * Обащение к семафору:
 * int semop(int semid, struct sembuf *sops, size_t nsops);
 * semid - уникальный номер для каждого набора семафор
 * sembuf - содержит 3 аргумента в своей структуре:
 *  sem_num - номер семафора 
 *  sem_op - операция над семафором. 
 *      -1, то процесс будет ожидать, пока другие процессы
 *          не освободят ресурс (очистки сета семафор sysctl)
 *      0, то процесс будет спать до тех пор, пока значение семафора не станет 0
 *      1, то ресурс отдает данному процессу
 *  sem_flg - флаг операции
 *      IPC_NOWAIT - default value
 *      SEM_UNDO - процесс будет автоматически отменен, после того, как он будет прерван
 * nsops - длина массива набора семафор
 * 
 */
int main()
{
    struct sembuf sem_lock = {0, -1, 0};
    struct sembuf sem_unlock = {0, 1, 0};
    key_t key;
    int sem_id, pid, n = 40, i = 0;
    char *path = "/tmp";
    int id = 'S';

    key = ftok(path, id);
    printf("Key value is %d\n", key);

    sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (sem_id == -1)
    {
        perror("semget:");
    }

    for (i = 0; i < n; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            semop(sem_id, &sem_lock, 1);
        }
        else if (pid > 0)
        {
            printf("n = %d, getpid() = %d\n", i, getpid());
            semop(sem_id, &sem_unlock, 1);
            exit(1);
        }
        else
        {
            perror("fork:");
            exit(1);
        }
    }
    if (semctl(sem_id, 0, IPC_RMID, 0) == -1)
    {
        perror("semctl:");
    }
    return 0;
}

//
// Created by msk on 12.06.16.
//

#define _GNU_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include "server.h"

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

volatile sig_atomic_t work = 1;

typedef struct
{
    int id;
    int *idlethreads;
    int *socket;
    pthread_cond_t *cond;
} thread_arg;

void siginthandler(int sig)
{
    work = 0;
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0x00, sizeof(struct sigaction));
    act.sa_handler = f;

    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;
        buf += c;
        len += c;
        count -= c;
    }
    while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int c;
    size_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if(c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    }
    while (count > 0);
    return len;
}

int make_socket(int domain, int type)
{
    int sock;
    sock = socket(domain, type, 0);
    if (sock < 0)
        ERR("socket");

    return sock;
}

char** read_from_databank(char *name) {

}

int validate_login(char **users, char *username, char *password) {

    return 0;
}

int check_for_user(char **users, char *username) {

}

int add_new_user(char *username, char *password) {

}

int handle_login(int clientfd) {
    ssize_t size;
    char buffer[CHUNKSIZE+1];

    if((size=bulk_read(clientfd,(char *)buffer,CHUNKSIZE+1))<0)
        ERR("read:");

    char **users;
    users = read_from_databank("users.db");

    if(check_for_user(users, buffer)>0) {

    }
    else {

    }
}

void communicate(int clientfd)
{
    ssize_t size;
    char command[COMM_SIZE+1];
    //char buffer[CHUNKSIZE];

    if ((size = TEMP_FAILURE_RETRY(recv(clientfd, command, COMM_SIZE + 1, MSG_WAITALL))) == -1)
        ERR("read");
    if (size == COMM_SIZE + 1)
    {
        if (strcmp(command, "login")) {
            if(handle_login(clientfd)>0) {

            }
            else {
                fprintf(stderr, "Incorrect username/password combination");
            }
        }
        else {
            fprintf(stderr, "Error in message!");
        }
    }
    if (TEMP_FAILURE_RETRY(close(clientfd)) < 0)
        ERR("close");
}

void *threadfunc(void *arg)
{
    int clientfd;
    thread_arg targ;

    memcpy(&targ, arg, sizeof(targ));

    while (1)
    {
        (*targ.idlethreads)++;
        if (!work)
            pthread_exit(NULL);
        (*targ.idlethreads)--;
        clientfd = *targ.socket;
        communicate(clientfd);
    }

    return NULL;
}

void cleanup(void *arg)
{

}

void init(pthread_t thread, thread_arg targ, int threadcount, int *idlethreads, int *socket)
{
    targ.id = threadcount + 1;
    targ.idlethreads = idlethreads;
    targ.socket = socket;
    if (pthread_create(&thread, NULL, threadfunc, (void *) &targ) != 0)
        ERR("pthread_create");
}

int bind_tcp_socket(uint16_t port)
{
    struct sockaddr_in addr;
    int socketfd, t=1;

    socketfd = make_socket(PF_INET, SOCK_STREAM);
    memset(&addr, 0x00, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
        ERR("setsockopt");
    if (bind(socketfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        ERR("bind");
    if (listen(socketfd, BACKLOG) < 0)
        ERR("listen");

    return socketfd;
}

int add_new_client(int sfd)
{
    int nfd;
    if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return -1;
        ERR("accept");
    }

    return nfd;
}

void dowork(int socket, pthread_t *thread, thread_arg *targ, int *idlethreads, int *cfd, sigset_t *oldmask)
{
    int clientfd;
    fd_set base_rfds, rfds;
    FD_ZERO(&base_rfds);
    FD_SET(socket, &base_rfds);

    while (work)
    {
        rfds = base_rfds;
        if (pselect(socket + 1, &rfds, NULL, NULL, NULL, oldmask) > 0)
        {
            if ((clientfd = add_new_client(socket)) == -1)
                continue;
            if (*idlethreads == 0)
            {
                if (TEMP_FAILURE_RETRY(close(clientfd)) == -1)
                    ERR("close");
            }
            else
            {
                *cfd = clientfd;
            }
        }
        else
        {
            if (EINTR == errno)
                continue;
            ERR("pselect");
        }
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s port\n",name);
    exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
    int i, socket, new_flags, cfd, idlethreads = 0;
    pthread_t thread[THREAD_MAX];
    thread_arg targ[THREAD_MAX];

    sigset_t mask, oldmask;

    if(argc!=2)
        usage(argv[0]);

    sethandler(SIG_IGN, SIGPIPE);
    sethandler(siginthandler, SIGINT);
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    socket = bind_tcp_socket(atoi(argv[1]));

    new_flags = fcntl(socket, F_GETFL) | O_NONBLOCK;

    if (fcntl(socket, F_SETFL, new_flags) == -1)
        ERR("fcntl");

    for (i = 0; i < THREAD_MAX; i++) {
        init(thread[i], targ[i], i, &idlethreads, &cfd);
    }

    dowork(socket, thread, targ, &idlethreads, &cfd, &oldmask);

    for (i = 0; i < THREAD_MAX; i++)
        if (pthread_join(thread[i], NULL) != 0)
            ERR("pthread_join");
    if (TEMP_FAILURE_RETRY(close(socket)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}
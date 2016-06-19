//
// Created by msk on 12.06.16.
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include "client.h"

#define ERR(source) (perror(source),\
             fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
             exit(EXIT_FAILURE))

typedef struct {
    struct sockaddr_in *addr;
    char *login;
    char *password;
} thread_arg;

volatile sig_atomic_t work = 1;
volatile sig_atomic_t authenticated = 0;

void sethandler(void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0x00, sizeof(struct sigaction));
    act.sa_handler = f;

    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void siginthandler(int sig) {
    work = 0;
}

ssize_t bulk_read(int fd, char *buf, size_t count) {
    int c;
    size_t len = 0;

    do {
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

ssize_t bulk_write(int fd, char *buf, size_t count) {
    int c;
    size_t len = 0;

    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    }
    while (count > 0);
    return len;
}

int make_socket(void) {
    int sock;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        ERR("socket");

    return sock;
}

struct sockaddr_in make_address(char *address, uint16_t port) {
    struct sockaddr_in addr;
    struct hostent *hostinfo;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    hostinfo = gethostbyname(address);
    if (hostinfo == NULL)
        ERR("gethostbyname");

    addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;

    return addr;
}

int connect_socket(struct sockaddr_in *addr) {
    int socketfd;
    socketfd = make_socket();
    if (connect(socketfd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) < 0) {
        if (errno != EINTR)
            ERR("connect");
        else {
            fd_set wfds;
            int status;
            socklen_t size = sizeof(int);
            FD_ZERO(&wfds);
            FD_SET(socketfd, &wfds);
            if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
                ERR("select");
            if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
                ERR("getsockopt");
            if (status != 0)
                ERR("connect");
        }
    }

    return socketfd;
}

void fetch_data(int sfd) {
    printf("Loading hot & nerdy singles in your area...\n");

}

int handle_response(char *buffer) {
    if (strcmp(buffer, "authok")) {
        authenticated = 1;
        return 0;
    }
    else if (strcmp(buffer, "authnok")) {
        printf("Username/password combination does not match!\n");
        return -1;
    }
    else {
        return -2;
    }
}

void *threadfunc(void *arg) {
    int fd;
    char buffer[CHUNKSIZE + 1];
    thread_arg *targ = (thread_arg *) arg;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
        ERR("pthread_mask");

    fd = connect_socket(targ->addr);

    if (bulk_write(fd, (void *) "login", COMM_SIZE + 1) < 0)
        ERR("write");
    if (bulk_write(fd, (void *) targ->login, CHUNKSIZE) < 0)
        ERR("write");
    if (bulk_write(fd, (void *) targ->password, CHUNKSIZE) < 0)
        ERR("write");
    if (bulk_read(fd, (void *) buffer, COMM_SIZE) < 0)
        ERR("read");

    if (handle_response(buffer) == 0) {
        fetch_data(fd);
    }

    if (TEMP_FAILURE_RETRY(close(fd)) == -1)
        ERR("close");
    if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0)
        ERR("pthread_sigmask");

    return NULL;
}

void dowork(struct sockaddr_in *addr, char *username, char *password) {
    thread_arg *targ;
    pthread_t thread;

    while (work) {
        if ((targ = (thread_arg *) calloc(1, sizeof(thread_arg))) == NULL)
            ERR("calloc");
        targ->addr = addr;
        targ->login = username;
        targ->password = password;

        if (pthread_create(&thread, NULL, threadfunc, (void *) targ) != 0)
            ERR("pthread_create");
        if (pthread_detach(thread) != 0)
            ERR("pthread_detach");
    }

    free(targ);
}

void usage(char *name) {
    fprintf(stderr, "USAGE: %s ip_address port\n", name);
    exit(EXIT_FAILURE);
}

void do_intro() {
    printf("_____/\\\\\\\\\\\\\\\\\\\\\\\\__/\\\\\\\\\\\\\\\\\\\\\\\\________/\\\\\\\\\\\\\\\\\\\\\\___        \n"
                   " ___/\\\\\\//////////__\\/\\\\\\////////\\\\\\____/\\\\\\/////////\\\\\\_       \n"
                   "  __/\\\\\\_____________\\/\\\\\\______\\//\\\\\\__\\//\\\\\\______\\///__      \n"
                   "   _\\/\\\\\\____/\\\\\\\\\\\\\\_\\/\\\\\\_______\\/\\\\\\___\\////\\\\\\_________     \n"
                   "    _\\/\\\\\\___\\/////\\\\\\_\\/\\\\\\_______\\/\\\\\\______\\////\\\\\\______    \n"
                   "     _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\_______\\/\\\\\\_________\\////\\\\\\___   \n"
                   "      _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\_______/\\\\\\___/\\\\\\______\\//\\\\\\__  \n"
                   "       _\\//\\\\\\\\\\\\\\\\\\\\\\\\/__\\/\\\\\\\\\\\\\\\\\\\\\\\\/___\\///\\\\\\\\\\\\\\\\\\\\\\/___ \n"
                   "        __\\////////////____\\////////////_______\\///////////_____");
    printf("\n        Welcome at the Geek Dating Service Client Application!\n");
}

int main(int argc, char **argv) {
    struct sockaddr_in addr;
    struct termios oflags, nflags;
    char username[64];
    char password[64];

    if (argc != 3)
        usage(argv[0]);

    sethandler(SIG_IGN, SIGPIPE);
    sethandler(siginthandler, SIGINT);

    do_intro();
    printf("Please input your login: ");
    scanf("%s", username);

    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        ERR("tcsetattr");
        return EXIT_FAILURE;
    }

    printf("\nPlease input your password for account %s: ", username);
    scanf("%s", password);

    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        ERR("tcsetattr");
        return EXIT_FAILURE;
    }

    addr = make_address(argv[1], atoi(argv[2]));
    dowork(&addr, username, password);

    return EXIT_SUCCESS;
}
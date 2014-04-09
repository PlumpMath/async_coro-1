/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | connect specified host:port infinitely                               |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |                                          |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-24 18:03                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include "acoro.h"

static in_addr_t ip;
static in_port_t port;

static const char *header = "GET /%d%d HTTP/1.0\r\nUser-Agent: curl/7.19.7\r\nHost: 122.11.56.5:8080\r\nAccept: */*\r\n\r\n";
static volatile int alive_cnt;

void *
connector(void *arg)
{
    int id = (intptr_t)arg;
    char buf[2048] = {0};
    size_t len;
    ssize_t nread;

    snprintf(buf, sizeof buf, header, rand(), rand());
    buf[sizeof buf - 1] = '\0';
    len = strlen(buf);

    printf("%d connecting\n", id);

    int fd = crt_tcp_blocked_connect(ip, port);
    if (fd < 0)
    {
        printf("fd = %d, err = %s\n", fd, strerror(crt_errno));
        abort();
    }

    printf("%d connected, writing\n", id);
    alive_cnt ++;

    ssize_t nwrite = crt_tcp_write_to(fd, buf, len, 1000 * 10);
    if (nwrite != (ssize_t)len)
    {
        printf("nwrite = %zd, err = %s\n", nwrite, strerror(crt_errno));
        abort();
    }

    printf("%d written successfully\n", id);

loop:
    nread = crt_tcp_read(fd, buf, 20);
    if (nread < 0)
    {
        printf("nread = %zd, err = %s\n", nread, strerror(crt_errno));
        abort();
    }
    buf[nread] = '\0';

    if (strstr(buf, "token") == NULL)
    {
        printf("[%d] got heartbeat(%zd): %s\n", id, nread, buf);
        goto loop;
    }
    else
    {
        printf("[%d] got message: %s\n", id, buf);
    }

    crt_sock_close(fd);

    alive_cnt --;

    crt_exit(NULL);
}

int
main(int argc, char **argv)
{
    init_coroutine_env(0);
    alive_cnt = 0;

    if (argc != 4)
    {
        printf("arg error\n");
        exit(-1);
    }

    ip = inet_addr(argv[1]);
    port = htons(atoi(argv[2]));
    int times = atoi(argv[3]);

    for (int i=0; i<times; i++)
    {
        crt_create(NULL, NULL, connector, (void*)(uintptr_t)i);
    }

    for ( ; ; )
    {
        sleep(10);
        printf("alive connections: %d\n", alive_cnt);
        if (alive_cnt == 0)
            exit(0);
    }

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */

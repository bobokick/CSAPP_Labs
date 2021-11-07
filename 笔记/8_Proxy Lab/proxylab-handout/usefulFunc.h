#ifndef USEFUL_FUNC_H
#define USEFUL_FUNC_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>

// 默认文件模式
#define DEF_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
// 默认RIO缓冲区大小
#define RIO_BUFFSIZE 8192

// 可重入的read函数
// 无错误则返回读取的字节数，如遇EOF返回0，错误返回-1
ssize_t rio_readn(int fd, char *usrbuff, size_t n)
{
    size_t rcnt = 0;
    ssize_t rnum;
    while (rcnt < n)
    {
        rnum = read(fd, usrbuff+rcnt, n);
        if (rnum == 0)
            break;
        else if (rnum == -1 && errno != EINTR)
            return -1;
        rcnt += rnum;
    }
    return rcnt;
}

// 可重入的write函数
// 无错误则返回写入的字节数，如遇EOF返回0，错误返回-1
ssize_t rio_writen(int fd, char *usrbuff, size_t n)
{
    size_t wcnt = 0;
    ssize_t wnum;
    while (wcnt < n)
    {
        wnum = write(fd, usrbuff+wcnt, n);
        if (wnum == -1 && errno != EINTR)
            return -1;
        wcnt += wnum;
    }
    return wcnt;
}

// RIO缓冲区结构
typedef struct
{
    int rio_fd;
    ssize_t rio_cnt;
    char *rio_buffPtr;
    char rio_buff[RIO_BUFFSIZE];
} rio_buff;

// 初始化RIO缓冲区
void rio_initBuff(rio_buff *pRioBuff, int fd)
{
    pRioBuff->rio_fd = fd;
    pRioBuff->rio_cnt = 0;
    pRioBuff->rio_buffPtr = pRioBuff->rio_buff;
}

// 带缓冲区的read函数，但只读部分值
// 该部分值为缓冲区已存在数据的大小和所需数据大小中的最小值
// 无错误则返回缓冲区已存在数据的大小和所需数据大小中的最小值，如遇EOF返回0，错误返回-1
static ssize_t rio_readb(rio_buff *pRioBuff, char *usrbuff, size_t n)
{
    size_t num;
    while (pRioBuff->rio_cnt <= 0)
    {
        pRioBuff->rio_cnt = read(pRioBuff->rio_fd, pRioBuff->rio_buff, sizeof(pRioBuff->rio_buff));
        if (pRioBuff->rio_cnt < 0 && errno != EINTR)
            return -1;
        else if (pRioBuff->rio_cnt == 0)
            return 0;
        else if (pRioBuff->rio_cnt > 0)
            pRioBuff->rio_buffPtr = pRioBuff->rio_buff;
    }
    num = pRioBuff->rio_cnt;
    if (num > n)
        num = n;
    memcpy(usrbuff, pRioBuff->rio_buffPtr, num);
    pRioBuff->rio_cnt -= num;
    pRioBuff->rio_buffPtr += num;
    return (ssize_t)num;
}

// 带缓冲区的可重入的read函数
// 无错误则返回读取的字节数，如遇EOF返回0，错误返回-1
ssize_t rio_readnb(rio_buff *pRioBuff, char *usrbuff, size_t n)
{
    size_t cnt = 0, num;
    while (cnt < n)
    {
        num = rio_readb(pRioBuff, usrbuff + cnt, n);
        if (num < 0)
            return -1;
        else if (num == 0)
            break;
        cnt += num;
    }
    return (ssize_t)cnt;
}

// 带缓冲区的可重入的readline，只读一行，
// 一行以'\n'结束，读取行时包括结尾的'\n'，并在结尾添加一个'\0'
// 无错误则返回读取的字节数，如遇EOF返回0，错误返回-1
ssize_t rio_readlineb(rio_buff *pRioBuff, char *usrbuff, size_t n)
{
    if (!n)
        return 0;
    char ch = '\0';
    size_t cnt = 0;
    ssize_t num;
    while (cnt < n-1)
    {
        if ((num = rio_readb(pRioBuff, &ch, 1)) == 0)
            break;
        else if (num == -1)
            return -1;
        usrbuff[cnt++] = ch;
        if (ch == '\n')
            break;
    }
    usrbuff[cnt] = '\0';
    return cnt;
}

// 可重入的readline函数，只读一行，
// 一行以'\n'结束，读取行时包括结尾的'\n'，并在结尾添加一个'\0'
// 无错误则返回读取的字节数，如遇EOF返回0，错误返回-1
ssize_t rio_readline(int fd, char *usrbuff, size_t n)
{
    if (!n)
        return 0;
    char ch = '\0';
    size_t cnt = 0;
    ssize_t num;
    while (cnt < n-1)
    {
        if ((num = rio_readn(fd, &ch, 1)) == 0)
            break;
        else if (num == -1)
            return -1;
        usrbuff[cnt++] = ch;
        if (ch == '\n')
            break;
    }
    usrbuff[cnt] = '\0';
    return cnt;
}

// 可重入的writeline函数，只写一行，
// 一行以'\0'结束，写入行时不写入结尾的'\0'
// 无错误则返回写入的字节数，如遇EOF返回0，错误返回-1
ssize_t rio_writeline(int fd, char *usrbuff, size_t n)
{
    size_t i = 0;
    for (; i < n; ++i)
        if (usrbuff[i] == '\0')
            break;
    return rio_writen(fd, usrbuff, i);
}

// 套接字连接函数，建立套接字并进行连接
// 无错误返回连接后的套接字，否则返回-1
int open_clientfd(char *hostname, char *port)
{
	// addrinfo结构
    struct addrinfo hints = {}, *res, *ptr;
    hints.ai_family = AF_INET;
	// tcp的传输方式
    hints.ai_socktype = SOCK_STREAM;
	// tcp协议
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_ADDRCONFIG;
	// 获取addrinfo用于sock和connect函数
    int addrInfoErr = getaddrinfo(hostname, port, &hints, &res);
	// 获取addrinfo错误显示
    if (addrInfoErr)
        printf("%s\n", gai_strerror(addrInfoErr));
    int clSock = -1, stcnct;
	// 使用addrinfo的每个块逐个进行sock和connect函数
    for (ptr = res; ptr; ptr = ptr->ai_next)
    {
        if ((clSock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) > 0 && !(stcnct = connect(clSock, ptr->ai_addr, ptr->ai_addrlen)))
            break;
        else
            close(clSock);
    }
    freeaddrinfo(res);
    // connect函数错误显示
    if (!ptr)
    {
        printf("connection failed!\n");
        if (stcnct == -1)
            perror("connect");
        return -1;
    }
    else
        return clSock;
}

// 套接字监听函数，建立套接字并进行绑定和监听
// 无错误返回监听后的套接字，否则返回-1
int open_listenfd(char *port)
{
	// addrinfo结构
    struct addrinfo hints = {}, *res, *ptr;
    hints.ai_family = AF_INET;
	// tcp的传输方式
    hints.ai_socktype = SOCK_STREAM;
	// tcp协议
    hints.ai_protocol = IPPROTO_TCP;
	// AI_PASSIVE用于设置通配符地址, 
	// 主机名应为NULL，告诉内核这个服务器会接受发送到该主机所有IP地址的请求。
    hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
    // 获取addrinfo用于sock和bind函数
    int addrInfoErr = getaddrinfo(NULL, port, &hints, &res);
	// 获取addrinfo错误显示
    if (addrInfoErr)
        printf("%s\n", gai_strerror(addrInfoErr));
    int listenfd;
    int stbind, stlsn, optval = 1;
	// 使用addrinfo的每个块逐个进行sock和bind函数
    for (ptr = res; ptr; ptr = ptr->ai_next)
    {
        if ((listenfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) > 0 && !setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) && !(stbind = bind(listenfd, ptr->ai_addr, ptr->ai_addrlen)) && !(stlsn = listen(listenfd, SOMAXCONN)))
            break;
        else
            close(listenfd);
    }
    freeaddrinfo(res);
	// sock和bind函数错误显示
    if (!ptr)
    {
        printf("start receiving server failed!\n");
        if (stbind == -1)
            perror("bind");
        if (stlsn == -1)
            perror("listen");
        return -1;
    }
    else
        return listenfd;
}

#endif
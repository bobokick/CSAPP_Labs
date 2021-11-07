#include "proxy.h"

// 代理的实现

cnntfdsQueue cnntfdQ;
int listenfd;

// SIGINT和SIGPIPE信号处理函数
void *sigKillProcess(int sig)
{
    close(listenfd);
    cleanCookies(&proxyCookies);
    deinitCfdsQueue(&cnntfdQ);
    printf("\nthe proxy server is terminated!\n");
    exit(0);
}

// 线程处理函数
void *threadAddress(void *argv)
{
    int cnntedfd;
    pthread_detach(pthread_self());
    while (1)
    {
        cnntedfd = getItemFromFdqueue(&cnntfdQ);
        addressRequest(cnntedfd);
        shutdown(cnntedfd, SHUT_RDWR);
    }
    return NULL;
}

// 命令行参数必须要有端口
int main(int argc, char **argv)
{
    // 无端口参数报错
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int cnntfd = -1, i;
    pthread_t pid;
    // 初始化代理缓冲区和描述符队列
    initCookies(&proxyCookies, MAX_CACHE_SIZE);
    initCfdsQueue(&cnntfdQ, MAX_ITEM_NUM);
    // 设置信号函数用于Ctrl+C和某方中断连接导致的BreakPipe
    signal(SIGINT, (__sighandler_t)sigKillProcess);
    signal(SIGINT, (__sighandler_t)sigKillProcess);
    // 创建监听fd
    listenfd = open_listenfd(argv[1]);
    // 创建8个线程来处理消息转发
    for (i = 0; i < 8; ++i)
        pthread_create(&pid, 0, threadAddress, NULL);
    printf("the proxy server is started, wait for proxy.\n");
    // 循环接收和处理代理请求
    while (1)
    {
        if ((cnntfd = accept(listenfd, NULL, NULL)) != -1)
            putItemToFdqueue(&cnntfdQ, cnntfd);
        else
            perror("accept");
    }
    close(listenfd);
    cleanCookies(&proxyCookies);
    deinitCfdsQueue(&cnntfdQ);
    return 0;
}

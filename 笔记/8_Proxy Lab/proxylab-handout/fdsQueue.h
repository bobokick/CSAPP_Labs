#ifndef FDQUEUE
#define FDQUEUE
#include "usefulFunc.h"

// 储存客户端描述符的队列结构，用于多线程
typedef struct
{
    // 队列大小
    int maxNum;
    // 队头元素的索引
    int front;
    // 队尾元素索引的下一个位置
    int end;
    // 储存队列元素
    int *fdsQueue;
    // 互斥锁，控制队列进出
    sem_t slotsLock;
    sem_t itemsLock;
    sem_t mutex;
} cnntfdsQueue;

// 初始化队列
// front%maxNum == end%maxNum时代表队列为空，
// front%maxNum == (end+1)%maxNum时代表队列满了
void initCfdsQueue(cnntfdsQueue *fdQueue, int number)
{
    fdQueue->maxNum = number;
    fdQueue->front = fdQueue->end = 0;
    fdQueue->fdsQueue = (int *)calloc(number, sizeof(int));
    sem_init(&fdQueue->slotsLock, 0, number);
    sem_init(&fdQueue->itemsLock, 0, 0);
    sem_init(&fdQueue->mutex, 0, 1);
}

// 清理队列结构，释放所用内存
void deinitCfdsQueue(cnntfdsQueue *fdQueue)
{
    free(fdQueue->fdsQueue);
}

// 将数据放入队列
void putItemToFdqueue(cnntfdsQueue *fdQueue, int cnntfds)
{
    sem_wait(&fdQueue->slotsLock);
    sem_wait(&fdQueue->mutex);
    fdQueue->fdsQueue[fdQueue->end++%fdQueue->maxNum] = cnntfds;
    sem_post(&fdQueue->mutex);
    sem_post(&fdQueue->itemsLock);
}

// 将数据从队列取出，并返回该数据
int getItemFromFdqueue(cnntfdsQueue *fdQueue)
{
    int tempfd;
    sem_wait(&fdQueue->itemsLock);
    sem_wait(&fdQueue->mutex);
    tempfd = fdQueue->fdsQueue[fdQueue->front++%fdQueue->maxNum];
    sem_post(&fdQueue->mutex);
    sem_post(&fdQueue->slotsLock);
    return tempfd;
}

#endif
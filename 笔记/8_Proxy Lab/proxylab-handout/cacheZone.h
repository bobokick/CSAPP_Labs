#ifndef CACHEZONE_H
#define CACHEZONE_H
#include "multilayerLlist.h"

// 常用缓冲区的实现（比如堆缓冲区）

// 统计缓冲区的malloc未释放的指针数量
int cacheMallocCnt = 0;

// 数据区结构，采用显式双向链表格式
struct buffNd
{
    // 标记该区是否有数据
    int status;
    // 数据区数据的起始位置
    char* startPos;
    // 该数据区大小
    int len;
    // 该区上次访问时间
    time_t clickTime;
    // 与数据相关的信息结构，用于对数据进行查找
    NodeOfMultilayerLlist *releInfo;
    // 上一节点
    struct buffNd *last;
    // 下一节点
    struct buffNd *next;
};
typedef struct buffNd nodeb;

// 缓冲区结构，维护数据区链表
typedef struct
{
    // 记录数据区的头节点
    nodeb *dataHead;
    // 记录数据区的当前空间最大的未保存数据的节点
    nodeb *maxsizeEdataNode;
    // 数据区节点相关信息的多层链表
    multilayerLlist *dataInfos;
    // 计时器
    time_t timeClock;
    // 缓冲区大小
    int cacheSize;
} Cache;

// static void *convertType(void *data, void *extra)
// {
//     int val = *((int *)extra);
//     return (nodeb*)data+val;
// }

// 初始化缓冲区结构
// 刚开始只有一个数据区，随着使用逐渐拆分和合并数据区
static void init_cache(Cache *cache, int cacheSize)
{
    cache->dataHead = (nodeb *)malloc(sizeof(nodeb)), ++cacheMallocCnt;
    cache->dataHead->status = 0;
    cache->dataHead->startPos = (char *)malloc(cacheSize), ++cacheMallocCnt;
    cache->dataHead->len = cacheSize;
    cache->dataHead->clickTime = -1;
    cache->dataHead->releInfo = NULL;
    cache->dataHead->last = NULL;
    cache->dataHead->next = NULL;
    cache->maxsizeEdataNode = cache->dataHead;
    cache->dataInfos = (multilayerLlist *)malloc(sizeof(multilayerLlist)), ++cacheMallocCnt;
    initMllist(cache->dataInfos, 3);
    cache->timeClock = 0;
    cache->cacheSize = cacheSize;
}

// 清理缓冲区，释放所用内存
static void deinit_cache(Cache *cache)
{
    nodeb *tempHead = cache->dataHead, *tempNode;
    deleteReleDataMll(cache->dataInfos, cache->dataInfos->topHead);
    free(cache->dataInfos), --cacheMallocCnt;
    free(tempHead->startPos), --cacheMallocCnt;
    while (tempHead->next != NULL)
    {
        tempNode = tempHead->next;
        free(tempHead), --cacheMallocCnt;
        tempHead = tempNode;
    }
    free(tempHead), --cacheMallocCnt;
}

// 将数据写入给定数据区，根据数据大小决定是否拆分数据区
// 并将pdata指向保存数据的数据区的指针
static void addDataToNode(nodeb *node, char *data, int dataLen, nodeb **pdata)
{
    int i;
    nodeb *tempNode;
    for (i = 0; i < dataLen; ++i)
        node->startPos[i] = data[i];
    node->status = 1;
    *pdata = node;
    if (node->len > dataLen)
    {
        tempNode = (nodeb *)malloc(sizeof(nodeb)), ++cacheMallocCnt;
        tempNode->status = 0;
        tempNode->startPos = node->startPos + i;
        tempNode->len = node->len - dataLen;
        tempNode->clickTime = -1;
        tempNode->releInfo = NULL;
        tempNode->last = node;
        tempNode->next = NULL;
        node->len = dataLen;
        if (node->next)
        {
            tempNode->next = node->next;
            node->next->last = tempNode;
        }
        node->next = tempNode;
    }
}

// 遍历缓冲区寻找当前空间最大的未保存数据的节点
static void maintainMaxSizeEnode(Cache *cache)
{
    nodeb *head = cache->dataHead, *tempNode = NULL;
    int maxsize = 0;
    for (; head != NULL; head = head->next)
        if (head->status == 0 && head->len > maxsize)
            tempNode = head, maxsize = head->len;
    cache->maxsizeEdataNode = (maxsize) ? tempNode : NULL;
}

// 将数据添加到缓冲区
// 并将pdata指向保存数据的数据区的指针
// 空间不够保存时返回-1，否则0
static int addDataToCache(Cache *cache, char *data, int dataLen, nodeb **pdata)
{
    nodeb *head = cache->dataHead;
    for (; head != NULL; head = head->next)
    {
        if (head->status == 0 && head->len >= dataLen)
        {
            addDataToNode(head, data, dataLen, pdata);
            head->clickTime = cache->timeClock++;
            maintainMaxSizeEnode(cache);
            break;
        }
    }
    if (head)
        return 0;
    else
        return -1;
}

// 检查数据区的上一节点和下一节点来判断是否要进行数据区合并
// 并返回合并后的数据区指针
static nodeb *mergeNode(nodeb *node)
{
    nodeb *tempNext = node->next, *tempLast = node->last;
    if (tempNext && tempNext->status == 0)
    {
        node->len += tempNext->len;
        node->next = tempNext->next;
        free(tempNext), --cacheMallocCnt;
    }
    if (tempLast && tempLast->status == 0)
    {
        tempLast->len += node->len;
        tempLast->next = node->next;
        free(node), --cacheMallocCnt;
        return tempLast;
    }
    else
        return node;
}

// 删除给定数据区中的数据，并检查是否要进行数据区合并和维护最大未使用空间
static void deleteDataFromNode(Cache *cache, nodeb *dataNode)
{
    nodeb *head = cache->dataHead, *tempNode;
    for (; head != NULL; head = head->next)
    {
        if (dataNode == head)
        {
            head->status = 0;
            if (head->releInfo)
                deleteReleDataMll(cache->dataInfos, head->releInfo);
            tempNode = mergeNode(head);
            if (!cache->maxsizeEdataNode || tempNode->len > cache->maxsizeEdataNode->len)
                cache->maxsizeEdataNode = tempNode;
            break;
        }
    }
}

// 查找最晚访问的数据区并返回其指针
static nodeb* searchEarliestNode(Cache *cache)
{
    nodeb *head = cache->dataHead, *tempNode = NULL;
    time_t earliestTime = __LONG_MAX__;
    for (; head != NULL; head = head->next)
        if (head->status == 1 && head->clickTime < earliestTime)
            tempNode = head, earliestTime = head->clickTime;
    if (earliestTime != __LONG_MAX__)
        return tempNode;
    else
        return NULL;
}

// 使用LRU策略来添加数据到缓冲区
// 当缓冲区空间不够时，根据策略释放不常用的数据来尽量满足空间来添加数据
// 空间超出缓冲区大小时返回-1，否则0
static int addDataToCacheByLRU(Cache *cache, char *data, int dataLen, nodeb **pdata)
{
    nodeb *tempNode = NULL;
    if (dataLen > cache->cacheSize)
        return -1;
    while (!cache->maxsizeEdataNode || cache->maxsizeEdataNode->len < dataLen)
    {
        tempNode = searchEarliestNode(cache);
        deleteDataFromNode(cache, tempNode);
    }
    return addDataToCache(cache, data, dataLen, pdata);
}

#endif
#ifndef COOKIEST_H
#define COOKIEST_H
#include "cacheZone.h"

// 代理缓冲区的实现

// 定义默认的代理缓冲区大小
#define DEFAULT_CACHE_SIZE 1049000

// 代理缓冲区结构，包装了常用缓冲区结构
typedef struct
{
    // 代理缓冲区是否初始化
    int initiated;
    // 当前读对象数量，使用多线程同步中的读者优先策略
    int creader;
    // 缓冲区结构
    Cache *pCache;
    // 互斥锁
    sem_t writerLock, mutex;
} Cookies;

// 初始化代理缓冲区结构
void initCookies(Cookies *cookie, int maxCacheSize)
{
    cookie->initiated = 1;
    cookie->creader = 0;
    cookie->pCache = (Cache *)malloc(sizeof(Cache));
    init_cache(cookie->pCache, maxCacheSize);
    sem_init(&cookie->writerLock, 0, 1);
    sem_init(&cookie->mutex, 0, 1);
}

// 清理代理缓冲区结构，释放所用内存
void cleanCookies(Cookies *cookie)
{
    cookie->initiated = 0;
    deinit_cache(cookie->pCache);
    free(cookie->pCache);
}

// 根据所给的相关数据信息providedInfo来查找代理缓冲区中的数据，
// 并将数据和其大小保存在storeData和dataLen
// 没有找到返回-1，否则0
int searchDataInCookie(Cookies *cookie, char **providedInfo, char **storeData, int *dataLen)
{
    if (!cookie->initiated)
        initCookies(cookie, DEFAULT_CACHE_SIZE);
    NodeOfMultilayerLlist *releData;
    nodeb *tempData;
    int res;
    sem_wait(&cookie->mutex);
    if (++cookie->creader == 1)
        sem_wait(&cookie->writerLock);
    sem_post(&cookie->mutex);
    if (!(res = searchNodeMll(cookie->pCache->dataInfos, providedInfo, &releData)))
    {
        tempData = (nodeb*)releData->extraData;
        tempData->clickTime = cookie->pCache->timeClock++;
        *storeData = tempData->startPos;
        *dataLen = tempData->len;
    }
    sem_wait(&cookie->mutex);
    if (--cookie->creader == 0)
        sem_post(&cookie->writerLock);
    sem_post(&cookie->mutex);
    return res;
}

// 使用LRU策略来添加数据到代理缓冲区，并根据该数据相关信息建立信息索引链表来方便查找该数据
// 添加失败返回-1，否则0
int addDataToCookie(Cookies *cookie, char *data, int dataLen, char **dataInfo)
{
    if (!cookie->initiated)
        initCookies(cookie, DEFAULT_CACHE_SIZE);
    nodeb *tempData;
    NodeOfMultilayerLlist *releInfo;
    int res;
    sem_wait(&cookie->writerLock);
    if (!(res = addDataToCacheByLRU(cookie->pCache, data, dataLen, &tempData)))
    {
        addDataMll(cookie->pCache->dataInfos, dataInfo, &releInfo);
        releInfo->extraData = tempData;
        tempData->releInfo = releInfo;
    }
    sem_post(&cookie->writerLock);
    return res;
}

#endif
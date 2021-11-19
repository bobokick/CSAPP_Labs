#ifndef WEBCACHE_H
#define WEBCACHE_H
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
} WebCache;

// 初始化代理缓冲区结构
void initWCache(WebCache *WCache, int maxCacheSize)
{
    WCache->initiated = 1;
    WCache->creader = 0;
    WCache->pCache = (Cache *)malloc(sizeof(Cache));
    init_cache(WCache->pCache, maxCacheSize);
    sem_init(&WCache->writerLock, 0, 1);
    sem_init(&WCache->mutex, 0, 1);
}

// 清理代理缓冲区结构，释放所用内存
void cleanWCache(WebCache *WCache)
{
    WCache->initiated = 0;
    deinit_cache(WCache->pCache);
    free(WCache->pCache);
}

// 根据所给的相关数据信息providedInfo来查找代理缓冲区中的数据，
// 并将数据和其大小保存在storeData和dataLen
// 没有找到返回-1，否则0
int searchDataInWCache(WebCache *WCache, char **providedInfo, char **storeData, int *dataLen)
{
    if (!WCache->initiated)
        initWCache(WCache, DEFAULT_CACHE_SIZE);
    NodeOfMultilayerLlist *releData;
    nodeb *tempData;
    int res;
    sem_wait(&WCache->mutex);
    if (++WCache->creader == 1)
        sem_wait(&WCache->writerLock);
    sem_post(&WCache->mutex);
    if (!(res = searchNodeMll(WCache->pCache->dataInfos, providedInfo, &releData)))
    {
        tempData = (nodeb*)releData->extraData;
        tempData->clickTime = WCache->pCache->timeClock++;
        *storeData = tempData->startPos;
        *dataLen = tempData->len;
    }
    sem_wait(&WCache->mutex);
    if (--WCache->creader == 0)
        sem_post(&WCache->writerLock);
    sem_post(&WCache->mutex);
    return res;
}

// 使用LRU策略来添加数据到代理缓冲区，并根据该数据相关信息建立信息索引链表来方便查找该数据
// 添加失败返回-1，否则0
int addDataToWCache(WebCache *WCache, char *data, int dataLen, char **dataInfo)
{
    if (!WCache->initiated)
        initWCache(WCache, DEFAULT_CACHE_SIZE);
    nodeb *tempData;
    NodeOfMultilayerLlist *releInfo;
    int res;
    sem_wait(&WCache->writerLock);
    if (!(res = addDataToCacheByLRU(WCache->pCache, data, dataLen, &tempData)))
    {
        addDataMll(WCache->pCache->dataInfos, dataInfo, &releInfo);
        releInfo->extraData = tempData;
        tempData->releInfo = releInfo;
    }
    sem_post(&WCache->writerLock);
    return res;
}

#endif
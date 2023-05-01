#ifndef MLTLLIST_H
#define MLTLLIST_H
#include "usefulFunc.h"

// 多层链表的实现

// 统计多层链表的malloc未释放的指针数量
int mllMallocCnt = 0;

// 多层链表节点结构，用该结构可构成多层链接的链表
struct mNode
{
    // 该节点所拥有的下层链表中的节点数量
    int numSubNodes;
    // 节点名字信息，用于对节点进行索引(键)
    char nodeName[50];
    // 节点保存的信息(值)
    void *extraData;
    // 该节点所属的上层节点
    struct mNode *upperNode;
    // 该节点所拥有的下层链表的表头节点
    struct mNode *subNode;
    // 上一节点
    struct mNode *last;
    // 下一节点
    struct mNode *next;
};
typedef struct mNode NodeOfMultilayerLlist;

// 多层链表结构
typedef struct
{
    // 层数
    int maxLayer;
    // 当前读对象数量，使用多线程同步中的读者优先策略
    int creader;
    // 最上层链表的头节点
    NodeOfMultilayerLlist *topHead;
    // 互斥锁
    sem_t writeLock, mutex;
} multilayerLlist;

// 初始化节点
static void initMnode(NodeOfMultilayerLlist *mnode)
{
    mnode->numSubNodes = 0;
    memset(mnode->nodeName, 0, sizeof(mnode->nodeName));
    mnode->extraData = NULL;
    mnode->upperNode = NULL;
    mnode->subNode = NULL;
    mnode->last = NULL;
    mnode->next = NULL;
}

// 删除某链表的某一节点，如果该节点是最上层链表的头节点，则使头节点指针指向新的头节点
static void deleteAmnode(NodeOfMultilayerLlist **head, NodeOfMultilayerLlist *mnode)
{
    if (mnode->next && mnode->last)
    {
        mnode->last->next = mnode->next;
        mnode->next->last = mnode->last;
    }
    else if (mnode->last)
        mnode->last->next = NULL;
    else if (mnode->next)
    {
        mnode->next->last = NULL;
        if (mnode == *head)
            *head = mnode->next;
        if (mnode->upperNode)
            mnode->upperNode->subNode = mnode->next;
    }
    else if (mnode == *head)
        *head = NULL;
    free(mnode), --mllMallocCnt;
}

// 删除该节点所拥有的链表的所有节点以及下属的链表
static void deleteSubMnode(NodeOfMultilayerLlist *mnode)
{
    int num = 0, stackSize = 10, single = 1, first = 1;
    // 栈结构，用于临时保存节点
    NodeOfMultilayerLlist **stack = (NodeOfMultilayerLlist **)calloc(sizeof(NodeOfMultilayerLlist**), stackSize);
    NodeOfMultilayerLlist *traverse = mnode, **tempStack, *tempNode;
    memset(stack, 0, stackSize);
    while (num || first)
    {
        first = 0;
        // 向下添加节点到栈
        for (; traverse != NULL; ++num, traverse = traverse->subNode)
        {
            if (num >= stackSize)
            {
                tempStack = (NodeOfMultilayerLlist **)calloc(sizeof(NodeOfMultilayerLlist**), stackSize*=2);
                memset(tempStack, 0, stackSize);
                memcpy(tempStack, stack, sizeof(NodeOfMultilayerLlist**)*stackSize);
                free(stack);
                stack = tempStack;
            }
            stack[num] = traverse;
        }
        if (num <= 1)
            break;
        traverse = stack[--num];
        // 删除头节点所属的链表
        while (traverse->next != NULL)
        {
            tempNode = traverse->next;
            free(traverse), --mllMallocCnt;
            traverse = tempNode;
        }
        free(traverse), --mllMallocCnt;
        // 向上寻找链表现存节点大于1的头部节点，并删除路径上的单一节点链表
        while (single && num > 1)
        {
            if (stack[num - 1]->next)
                traverse = stack[num - 1]->next, single = 0;
            free(stack[num - 1]), --mllMallocCnt;
            --num;
        }
        if (num <= 1 && single)
            break;
        single = 1;
    }
    free(stack);
}

// 从多层链表中删除给定节点相关的所有链表节点以及该节点
// 也就是删除该节点的下层链表以及相关的上层单一节点
// 给定节点为空返回-1，否则0
static int deleteRelevantMnodeFromMllist(NodeOfMultilayerLlist **head, NodeOfMultilayerLlist *needDeletedNode)
{
    NodeOfMultilayerLlist *traverse = needDeletedNode, *tempNode;
    if (traverse)
    {
        deleteSubMnode(traverse);
        while (traverse->upperNode != NULL && traverse->upperNode->numSubNodes == 1)
        {
            tempNode = traverse->upperNode;
            free(traverse), --mllMallocCnt;
            traverse = tempNode;
        }
        if (traverse->upperNode != NULL)
            traverse->upperNode->numSubNodes -= 1;
        deleteAmnode(head, traverse);
        return 0;
    }
    else
        return -1;
}

// 根据给定的键添加数据到多层链表中，形成上下链接的数据链，每个数据可关联多个子数据形成的链表
// 并在bottomNodeForSaving中保存最子数据节点的指针
static void addDataToMllist(NodeOfMultilayerLlist **head, char **nodeNames, void **extraData, void *(*addressExtraData)(void *, void *) , int layer, NodeOfMultilayerLlist **bottomNodeForSaving)
{
    int i, exist = 0, skip1 = 1;
    NodeOfMultilayerLlist *traverse = *head, *lastNode = NULL, *tempNode = NULL, *tempHead = NULL;
    // 根据层数添加相应数据
    for (i = 0; i < layer; ++i)
    {
        // 在多层链表中寻找是否有对应的键(名字)，有就往下层继续找
        for (; traverse != NULL; lastNode = traverse, traverse = traverse->next)
            if (!strcmp(nodeNames[i], traverse->nodeName))
            {
                if (extraData && addressExtraData)
                    traverse->extraData = addressExtraData(*extraData, (void*)&i);
                lastNode = traverse;
                traverse = traverse->subNode;
                exist = 1;
                break;
            }
        // 在某层如果没有对应键，则开始新建键和添加值
        if (!traverse)
        {
            // 情况1：该层有键但该键没有下层链表
            if (skip1 && exist)
            {
                skip1 = 0;
                tempNode = lastNode;
                continue;
            }
            else if (!tempNode)
            {   
                // 情况2：该层没有键
                if (lastNode)
                {
                    tempHead = lastNode;
                    while (tempHead->last != NULL)
                        tempHead = tempHead->last;
                    if (tempHead->upperNode)
                        tempHead->upperNode->numSubNodes += 1;
                    lastNode->next = (NodeOfMultilayerLlist*)malloc(sizeof(NodeOfMultilayerLlist)), ++mllMallocCnt;
                    initMnode(lastNode->next);
                    lastNode->next->last = lastNode;
                    lastNode->next->upperNode = tempHead->upperNode;
                    tempNode = lastNode->next;
                }
                // 情况3：多层链表不存在，头节点为空
                else
                {
                    tempNode = (NodeOfMultilayerLlist*)malloc(sizeof(NodeOfMultilayerLlist)), ++mllMallocCnt;
                    *head = tempNode;
                    initMnode(tempNode);
                }
            }
            // 新建键和添加值
            else
            {
                tempNode->subNode = (NodeOfMultilayerLlist*)malloc(sizeof(NodeOfMultilayerLlist)), ++mllMallocCnt;
                initMnode(tempNode->subNode);
                tempNode->numSubNodes += 1;
                tempNode->subNode->upperNode = tempNode;
                tempNode = tempNode->subNode;
            }
            strcpy(tempNode->nodeName, nodeNames[i]);
            if (extraData && addressExtraData)
                tempNode->extraData = addressExtraData(*extraData, &i);
        }
        else
            exist = 0;
    }
    // 对应键如果都有的情况
    if (traverse)
        *bottomNodeForSaving = lastNode;
    // 新建键的情况
    else
        *bottomNodeForSaving = tempNode;
}

// 在多层链表中根据给定的键寻找节点，并在bottomNodeForSaving中保存该节点的指针
// 没有找到返回-1，否则0
static int searchDataInMllist(NodeOfMultilayerLlist *head, char **nodeNames, int layer, NodeOfMultilayerLlist **bottomNodeForSaving)
{
    int i;
    NodeOfMultilayerLlist *traverse = head;
    for (i = 0; i < layer; ++i)
    {
        for (; traverse != NULL; traverse = traverse->next)
            if (!strcmp(nodeNames[i], traverse->nodeName))
            {
                if (i < layer - 1)
                    traverse = traverse->subNode;
                break;
            }
    }
    if (traverse)
    {
        *bottomNodeForSaving = traverse;
        return 0;
    }
    else
        return -1;
}

// 包装函数
// 删除多层链表给定节点相关的所有链表节点以及该节点
// 也就是删除该节点的下层链表以及相关的上层单一节点
// 给定节点为空返回-1，否则0
int deleteReleDataMll(multilayerLlist *mllist, NodeOfMultilayerLlist *needDeletedNode)
{
    int res;
    sem_wait(&mllist->writeLock);
    res = deleteRelevantMnodeFromMllist(&mllist->topHead, needDeletedNode);
    sem_post(&mllist->writeLock);
    return res;
}

// 包装函数
// 根据给定的键添加数据到多层链表中
// 形成上下链接的数据链，每个数据可关联多个子数据形成的链表
// 并在bottomNodeForSaving中保存最子数据节点的指针
// 给定节点为空返回-1，否则0
void addDataMll(multilayerLlist *mllist, char **nodeNames, NodeOfMultilayerLlist **bottomNodeForSaving)
{
    sem_wait(&mllist->writeLock);
    addDataToMllist(&mllist->topHead, nodeNames, NULL, NULL, mllist->maxLayer, bottomNodeForSaving);
    sem_post(&mllist->writeLock);
}

// 包装函数
// 在多层链表中根据给定的键寻找节点，并在bottomNodeForSaving中保存该节点的指针
// 没有找到返回-1，否则0
int searchNodeMll(multilayerLlist *mllist, char **nodeNames, NodeOfMultilayerLlist **bottomNodeForSaving)
{
    int res;
    sem_wait(&mllist->mutex);
        if (++mllist->creader == 1)
            sem_wait(&mllist->writeLock);
    sem_post(&mllist->mutex);
    res = searchDataInMllist(mllist->topHead, nodeNames, mllist->maxLayer, bottomNodeForSaving);
    sem_wait(&mllist->mutex);
        if (--mllist->creader == 0)
            sem_post(&mllist->writeLock);
    sem_post(&mllist->mutex);
    return res;
}

// 初始化多层链表
void initMllist(multilayerLlist *mllist, int maxLayer)
{
    mllist->maxLayer = maxLayer;
    mllist->creader = 0;
    mllist->topHead = NULL;
    sem_init(&mllist->writeLock, 0, 1);
    sem_init(&mllist->mutex, 0, 1);
}

// 清理多层链表，释放所用内存
void deinitMllist(multilayerLlist *mllist)
{
    deleteReleDataMll(mllist, mllist->topHead);
}

#endif
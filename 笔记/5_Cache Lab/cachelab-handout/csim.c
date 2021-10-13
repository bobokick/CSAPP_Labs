#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// maked by bbk

// 储存命令行参数
typedef struct
{
    int status;
    int verbose;
    int s;
    int E;
    int b;
    FILE *file;
} argInf;

// 储存内存操作与地址(二进制)
typedef struct 
{
    char method;
    char addr_2r[40];
} memInf;

// 储存命中结果
typedef struct
{
    char memNum[20];
    char result[10];
    char extraInf[15];
} memAccRes;

// 高速缓存器结构
typedef struct
{
    int valid;
    int flag;
    unsigned mTime;
} cacheInf;

// 储存翻译内存操作后的索引组id和标志位id
typedef struct
{
    int groupId;
    int flag;
} addrInf;

// 格式化内存操作字符串，用于输出命中结果
void stringAddress(char str[], int len)
{
    int lastPos = len-1, endPrezero = 3, startRe = 3;
    for (; lastPos >= 0 && (str[lastPos] < '0' || str[lastPos] > '9'); --lastPos)
        str[lastPos] = '\0';
    for (; endPrezero < len && str[endPrezero] == '0'; ++endPrezero);
    if (endPrezero != 3)
    {
        for (; endPrezero <= lastPos; ++startRe, ++endPrezero)
            str[startRe] = str[endPrezero];
        while (startRe < endPrezero)
            str[startRe++] = '\0';
    }
}

// 输出命令行操作提示
void printfInf(int order, char *inf)
{
    char baseInf[] = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
    -h         Print this help message.\n\
    -v         Optional verbose flag.\n\
    -s <num>   Number of set index bits.\n\
    -E <num>   Number of lines per set.\n\
    -b <num>   Number of block offset bits.\n\
    -t <file>  Trace file.\n\n\
Examples:\n\
linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n\
linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";
    switch (order)
    {
    case 0:
        printf("%s",inf);
        printf("%s\n",baseInf);
        break;
    case 1:
        printf("option requires an argument -- %s\n", inf);
        printf("%s\n",baseInf);
        break;
    case 2:
        printf("invalid options -- %s\n", inf);
        printf("%s\n",baseInf);
        break;
    case 3:
        printf("%s: No such file or directory\n", inf);
        break;
    default:
        break;
    }
}

// 检查命令行参数是否符合要求
argInf chkArgs(int argc, char *args[])
{
    argInf argInfo = {0,0,0,0,0,0};
    int order = 0;
    char *errInf = "Missing required command line argument\n";
    FILE *tempFile = NULL;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp("-h",args[i]))
        {
            errInf = "";
            break;
        }
        else if (!(strcmp("-s",args[i]) && strcmp("-E",args[i]) && strcmp("-b",args[i]) && strcmp("-t",args[i])))
        {
            if (i + 1 >= argc || (*args[i+1] == '-' && atoi(args[i+1]) == 0))
            {
                order = 1, errInf = args[i];
                break;
            }
            else if (strcmp("-t",args[i]) && atoi(args[i+1]) <= 0)
            {
                order = 2, errInf = args[i+1];
                break;
            }
            else if (!strcmp("-t",args[i]) && (tempFile = fopen(args[i+1],"r")) == 0)
            {
                order = 3, errInf = args[i+1];
                break;
            }

            if (!(strcmp("-s",args[i])))
                argInfo.s = atoi(args[i+1]);
            else if (!(strcmp("-E",args[i])))
                argInfo.E = atoi(args[i+1]);
            else if (!(strcmp("-b",args[i])))
                argInfo.b = atoi(args[i+1]);
            else if (!(strcmp("-t",args[i])))
                argInfo.file = tempFile;
        }
        else if (!(strcmp("-v",args[i])))
            argInfo.verbose = 1;
    }
    if (argInfo.s != 0 && argInfo.E != 0 && argInfo.b != 0 && argInfo.file != 0)
        argInfo.status = 1;
    else
        printfInf(order, errInf);
    return argInfo;
}

// 16进制字符串转2进制字符串
void r16tor2(char *store, char *src)
{
    for (int i = 0, count = 0; src[i] != 0; ++i)
        switch (src[i])
        {
            case '0':
                strcpy(store+count, "0000"), count += 4;
                break;
            case '1':
                strcpy(store+count, "0001"), count += 4;
                break;
            case '2':
                strcpy(store+count, "0010"), count += 4;
                break;
            case '3':
                strcpy(store+count, "0011"), count += 4;
                break;
            case '4':
                strcpy(store+count, "0100"), count += 4;
                break;
            case '5':
                strcpy(store+count, "0101"), count += 4;
                break;
            case '6':
                strcpy(store+count, "0110"), count += 4;
                break;
            case '7':
                strcpy(store+count, "0111"), count += 4;
                break;
            case '8':
                strcpy(store+count, "1000"), count += 4;
                break;
            case '9':
                strcpy(store+count, "1001"), count += 4;
                break;
            case 'a':
            case 'A':
                strcpy(store+count, "1010"), count += 4;
                break;
            case 'b':
            case 'B':
                strcpy(store+count, "1011"), count += 4;
                break;
            case 'c':
            case 'C':
                strcpy(store+count, "1100"), count += 4;
                break;
            case 'd':
            case 'D':
                strcpy(store+count, "1101"), count += 4;
                break;
            case 'e':
            case 'E':
                strcpy(store+count, "1110"), count += 4;
                break;
            case 'f':
            case 'F':
                strcpy(store+count, "1111"), count += 4;
                break;
            default:
                break;
        }
    
}

// 初始化字符数组
void initiateCharArray(char *arr, int len)
{
    for (int i = 0; i < len; ++i)
        arr[i] = '\0';
}

// 初始化高速缓存器结构数组
void initiateCacheInfArray(cacheInf *arr, int len)
{
    for (int i = 0; i < len; ++i)
        arr[i].valid = 0, arr[i].flag = 0, arr[i].mTime = __UINT32_MAX__;
}

// 从文本提取内存操作与地址(二进制)
memInf fetchInf(char *text)
{
    memInf memoInfo;
    initiateCharArray(memoInfo.addr_2r, 40);
    char orgAddr[20] = "";
    memoInfo.method = text[1];
    for (int i = 3; text[i] != ','; ++i)
        orgAddr[i-3]=text[i];
    r16tor2(memoInfo.addr_2r, orgAddr);
    return memoInfo;
}

// 翻译内存地址为索引组id和标志位id
addrInf transAddr(char *addr_2r, int s, int b)
{
    addrInf res = {0,0};
    int endPos = 0;
    char num_s[s+1];
    for (; addr_2r[endPos] != 0; ++endPos);
    char num_t[endPos-s-b+1];
    for (int i = endPos-s-b, c = 0; i < endPos - b; ++i, ++c)
        num_s[c] = addr_2r[i];
    for (int i = 0; i < endPos-s-b; ++i)
        num_t[i] = addr_2r[i];
    num_s[s] = num_t[endPos-s-b] = 0;
    res.groupId = strtol(num_s, 0, 2);
    res.flag = strtol(num_t, 0, 2);
    return res;
}

// 替换策略LRU
int evictionSelection_LRU(cacheInf group[], int E)
{
    unsigned tempTime = __UINT32_MAX__;
    int pos = -1;
    for (int i = 0; i < E; ++i)
        if (group[i].mTime < tempTime)
            tempTime = group[i].mTime, pos = i;
    return pos;
}

// 检查是否命中缓存
int hitChk(cacheInf group[], int E, int flag)
{
    for (int i = 0; i < E; ++i)
        if (group[i].valid == 1 && group[i].flag == flag)
            return i;
    return -1;
}

// 检查给定组中是否有空白位置
int spaceChk(cacheInf group[], int E)
{
    for (int i = 0; i < E; ++i)
        if (group[i].valid != 1)
            return i;
    return -1;
}

// 计时缓存操作
unsigned countClock()
{
    static unsigned time = 0;
    return time++;
}

// 主函数
int main(int argc, char *args[])
{
    argInf argInfo = chkArgs(argc, args);
    if (argInfo.status == 0)
        return 0;
    // 建立并初始化高速缓存器
    cacheInf cacheDepot[(int)pow(2, argInfo.s)][argInfo.E];
    for (int i = 0; i < (int)pow(2, argInfo.s); ++i)
        initiateCacheInfArray(cacheDepot[i], argInfo.E);
    int cHit = 0, cMiss = 0, cEviction = 0;
    char strs[20] = "";
    memInf memInfo;
    memAccRes memResult;
    addrInf tempAddrInfo = {0,0};
    // 执行文本中的内存操作
    for (int tempPos = 0; fgets(strs,20,argInfo.file);)
    {
        // 获取内存操作与地址(二进制)
        if (strs[0] == ' ')
        {
            initiateCharArray(memInfo.addr_2r, 40);
            initiateCharArray(memResult.memNum, 20);
            initiateCharArray(memResult.result, 10);
            initiateCharArray(memResult.extraInf, 15);
            memInfo = fetchInf(strs);
            stringAddress(strs, 20);
            strcpy(memResult.memNum, strs+1);
        }
        else 
            continue;
        // 翻译地址
        tempAddrInfo = transAddr(memInfo.addr_2r, argInfo.s, argInfo.b);
        // 检查该操作是否命中缓存，并记录对应的结果
        tempPos = hitChk(cacheDepot[tempAddrInfo.groupId], argInfo.E, tempAddrInfo.flag);
        if (tempPos != -1)
            ++cHit, strcpy(memResult.result, "hit"), strcpy(memResult.extraInf, "");
        else
        {
            tempPos = spaceChk(cacheDepot[tempAddrInfo.groupId], argInfo.E);
            if (tempPos == -1)
            {
                tempPos = evictionSelection_LRU(cacheDepot[tempAddrInfo.groupId], argInfo.E);
                ++cEviction, strcpy(memResult.extraInf, "eviction");
            }
            cacheDepot[tempAddrInfo.groupId][tempPos].valid = 1;
            cacheDepot[tempAddrInfo.groupId][tempPos].flag = tempAddrInfo.flag;
            ++cMiss, strcpy(memResult.result, "miss");
        }
        cacheDepot[tempAddrInfo.groupId][tempPos].mTime = countClock();
        if (memInfo.method == 'M')
        {
            ++cHit;
            if (memResult.extraInf[0] != '\0')
                strcat(memResult.extraInf, " ");
            strcat(memResult.extraInf, "hit");
        }
        // 输出命中结果
        if (argInfo.verbose == 1)
            printf("%s %s %s\n", memResult.memNum, memResult.result, memResult.extraInf);
        initiateCharArray(strs, 20);
    }
    // 输出统计的结果
    printf("hits:%d misses:%d evictions:%d\n", cHit, cMiss, cEviction);
    printSummary(cHit, cMiss, cEviction);
    return 0;
}


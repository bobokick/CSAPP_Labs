## Cache Lab实验解答

### 1.实验概述

> 注意：Cache Lab实验需要用到CSAPP_3th第六章的高速缓存存储器方面的内容和基本的C语言语法知识。
> 所以实验之前需要掌握第六章和C语言语法的知识。

本次实验的内容是让我们了解计算机缓冲存储器的逻辑设计和实现方法，并能够根据其设计进行代码的优化，以便能够充分使用到缓存存储器，从而写出更快更好的程序。

这次实验一共有2关，第1关让我们实现计算机高速缓存存储器的逻辑设计，第2关让我们根据高速缓存存储器的特性，写出一个充分利用了高速缓存存储器的矩阵转置算法。

实验还提供了每一关对应的示例与测试等工具。

### 2.实验准备

本次实验所包含的文档`cachelab.pdf`以及文件夹中的`README`文档详细介绍了每一关所需要进行的步骤、完成的目标和附带工具的使用方法，以供我们参考。
我们所用的机器需要有C语言环境，以便对我们写的C语言代码进行编译链接等操作。

### 3.进行实验

本次实验主要是对CSAPP_3th第六章所涉及的高速缓存存储器相关知识的运用。

**实验相关知识**

在进行实验之前，我们需要了解高速缓存存储器的相关知识，以下是书中有关高速缓存存储器的一些讲解：
![k1](image/2021-10-13%2021-22-11屏幕截图.png)
![k2](image/2021-10-13%2021-22-23屏幕截图.png)
![k3](image/2021-10-13%2021-22-30屏幕截图.png)

#### 3.1 第一关

##### 3.11 任务要求

第一关所需的所有工具文件在`cachelab-handout/`目录下。

本关给我们一个高速缓存存储器示例程序`csim-ref`，该程序可以通过读取`cachelab-handout/traces`目录下的内存操作记录文件（这些内存操作文件是由一个Linux程序`valgrind`来记录的，内存操作有四种类型，每种类型的格式基本都是` 操作类型 内存地址,操作内存大小`），通过给定的命令行参数来分析每个内存操作是否命中了缓存，该程序还会统计总的命中次数等信息。
![1.1](image/2021-10-13%2021-40-05屏幕截图.png)
以下为示例程序`csim-ref`的操作参数：
![1.2](image/2021-10-13%2021-42-09屏幕截图.png)

> 在这一关，我们不需要对内存操作中的指令内存操作进行分析（也就是`I`开头的记录），也不需要考虑操作内存大小（也就是忽略`，`之后的内存大小记录）。
> 本关所写的代码中不能有任何的Error和Warning。

我们需要在文件`csim.c`中模仿该程序写出一个操作相同，分析逻辑相同的高速缓存存储器分析程序，并通过测试才算完成该关。

我们在编写完文件`csim.c`后，需要在`main`函数的最后，调用函数`printSummary(int hit_count, int miss_count, int eviction_count);`来使之后的程序测试正常运行。
编写完文件`csim.c`后，在目录为`cachelab-handout/`的命令行下输入`make`来自动编译程序，然后接着输入`./test-csim`来测试我们所制作的程序，测试部分会测试多个不同的参数组合来验证我们的程序，我们需要与示例程序`csim-ref`的分析结果全部相同才能满分。
![1.3](image/2021-10-13%2021-55-50屏幕截图.png)

##### 3.12 任务解答

我们打开文件`csim.c`发现只有一个`main`函数，所以这关需要我们从头开始实现高速缓存存储器分析程序。

1. 第一步：
我们首先要实现和示例程序一样的接受和验证命令行参数的功能，通过基本的C语言语法我们知道要在函数`main`的原型中写上相应的命令行行参，这样才能够接受命令行参数字符串，我们还需要对这些命令行参数进行判断和处理。以下是我编写的关于处理命令行参数方面的函数代码：
```c
// 以下代码在文件csim.c中

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
```

2. 第二步：
实现了命令行参数功能后，我们需要根据提供的参数搭建相应的高速缓存存储器：

```c
// 以下代码在文件csim.c中

// 高速缓存器结构
typedef struct
{
    int valid;
    int flag;
    unsigned mTime;
} cacheInf;

// 初始化高速缓存器结构数组
void initiateCacheInfArray(cacheInf *arr, int len)
{
    for (int i = 0; i < len; ++i)
        arr[i].valid = 0, arr[i].flag = 0, arr[i].mTime = __UINT32_MAX__;
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
}
```

3. 第三步：
然后就是提取内存操作记录文件的相关信息，并将内存操作地址翻译成对应的索引组id和标志位id等信息：
```c
// 以下代码在文件csim.c中

// 储存内存操作与地址(二进制)
typedef struct 
{
    char method;
    char addr_2r[40];
} memInf;

// 储存翻译内存操作后的索引组id和标志位id
typedef struct
{
    int groupId;
    int flag;
} addrInf;

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
```

4. 最后一步：
根据翻译出的索引组id和标志位id等信息对高速缓存存储器执行对应的操作，并记录和输出相关数据结果：
```c
// 以下代码在文件csim.c中

// 储存命中结果
typedef struct
{
    char memNum[20];
    char result[10];
    char extraInf[15];
} memAccRes;

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

// 初始化字符数组
void initiateCharArray(char *arr, int len)
{
    for (int i = 0; i < len; ++i)
        arr[i] = '\0';
}

// 主函数
int main(int argc, char *args[])
{
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
```

以下是该代码编译出的高速缓存存储器分析程序测试通过的界面（满分）：
![pass1](image/2021-10-13%2021-55-50屏幕截图.png)

#### 3.2 第二关

##### 3.21 任务要求

第二关所需的所有工具文件也在`cachelab-handout/`目录下。

本关让我们在文件`trans.c`中写出一个矩阵转置算法，并使该算法充分利用高速缓存存储器的特性，对缓存不命中的效果达到尽可能的小。

> 在这一关，我们有一些限制条件：
> 1. 本关所写的代码中不能有任何的Error和Warning。
> 2. 本关不能使用除了int型变量的其他，比如long、数组等类型的变量，且int型变量不能超过12个。
> 3. 本关不能修改变量A。
> 4. 本关不能使用递归。
> 5. 本关可以使用其他函数，但是这些函数的行参也要遵守规则2。

我们在编写完文件`trans.c`后，在目录为`cachelab-handout/`的命令行下输入`make`来自动编译程序，然后接着输入`./test-trans -M 32 -N 32`，`./test-trans -M 64 -N 64`和`./test-trans -M 61 -N 67`来测试我们所制作的程序，以下是根据测试结果中的缓存不命中次数来计分的计分规则。
![2.1](image/2021-10-13%2022-37-31屏幕截图.png)

> 本关所用的高速缓存存储器的大小为`s = 5, E = 1, b = 5`，也就是每个高速缓存块的大小为32Byte，缓存存储器总大小为1KB。

##### 3.22 任务解答

我们打开文件`trans.c`，发现我们只需要实现函数`transpose_submit`，其他的函数不能修改，在该文件中有一个矩阵转置算法的示例函数`trans`，我们可以试试这个示例函数，看看默认测试结果是多少。
![2.2](image/2021-10-13%2022-50-11屏幕截图.png)

默认测试结果离目标很远，所以我们不可能直接用示例函数就能过关，此时我们可以想一想矩阵方面的知识，发现可以用矩阵分块来减小缓存不命中次数。
我们将一个矩阵分成多块，从上到下，从左到右依次计算小的矩阵，这样在计算小的矩阵时能够充分利用高速缓存存储器存储中的数据文件，提高缓存命中次数。

那么我们需要分多少块呢，考虑到每个高速缓存块的大小为32Byte，缓存存储器总大小为1KB，我们的块数以大矩阵于1024KB的倍数的平方最好。

以下为我编写的利用了矩阵分块特性的矩阵转置算法：
```c
// 以下代码在文件trans.c中

void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    // 使用分块矩阵，块数以该矩阵于1024KB的倍数的平方最好
    int cutNum = N*M*4/1024.0+0.1;
    int i, j, tmp, PartRowlen = N/cutNum, PartColumnlen = N/cutNum;
    for (int cRow = 0, rowLen = (cRow+1)*PartRowlen; cRow < cutNum; ++cRow, rowLen = ((cRow == cutNum-1) ? N : (cRow+1)*PartRowlen))
        for (int cColumn = 0, columnLen = (cColumn+1)*PartColumnlen; cColumn < cutNum; ++cColumn, columnLen = ((cColumn == cutNum-1) ? N : (cColumn+1)*PartColumnlen))
            for (i = cRow*PartRowlen; i < rowLen; i++) 
            {
                for (j = cColumn*PartColumnlen; j < columnLen; j++)
                {
                    tmp = A[i][j];
                    B[j][i] = tmp;
                }
            }
}
```

以下是该代码编译出的矩阵转置算法测试通过的界面（非满分）：
![pass2](image/2021-10-13%2023-04-38屏幕截图.png)

### 4.总结

本次实验让我们深入地了解了计算机高速缓冲存储器的逻辑设计和实现方法，通过这次实验，我们在以后的编写代码时能够从高速缓冲存储器的方面考虑代码编写所花费的时间与空间，这样能够让我们写出时间更快，内存更少的程序。

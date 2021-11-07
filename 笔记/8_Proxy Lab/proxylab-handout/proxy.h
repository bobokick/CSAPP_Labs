#ifndef PROXY_H
#define PROXY_H
#include "usefulFunc.h"
#include "fdsQueue.h"
#include "cookieStruct.h"

// 描述符队列的大小
#define MAX_ITEM_NUM 20
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

Cookies proxyCookies;

// 请求行结构
typedef struct
{
    char method[10];
    char hostname[40];
    char uri[88];
    char ver[20];
    char port[6];
} requestLSet;

// 初始化请求行
void initRequeseLine(requestLSet* pRlset)
{
    memset(pRlset, 0, sizeof(requestLSet));
}

// 语法分析请求行
static int parse_string(char *str, requestLSet* pstore)
{
    int cSpace = 0, cSlash = 0, i, cmethod, chostname, curi, cport, cver;
    for (i = cmethod = chostname = curi = cport = cver = 0; str[i] != '\r'; ++i)
    {
        cSpace += (str[i] == ' ') ? 1 : 0;
        cSlash += (str[i] == '/') ? 1 : 0;
        // 提取方法信息
        if (cSpace < 1)
            *(pstore->method+cmethod++) = str[i];
        // 提取域名/IP信息
        else if (i > 2 && cSpace < 2 && str[i-3] == ':' && str[i-2] == '/' && str[i-1] == '/')
        {
            while (str[i+chostname] != '/' && str[i+chostname] != ':')
                *(pstore->hostname+chostname) = str[i+chostname], ++chostname;
            i += (chostname > 0) ? chostname - 1 : 0;
        }
        // 提取端口信息
        else if (i > 0 && cSpace < 2 && str[i-1] == ':' && str[i] >= '0' && str[i] <= '9')
        {
            while (str[i+cport] >= '0' && str[i+cport] <= '9' )
                *(pstore->port+cport) = str[i+cport], ++cport;
            i += (cport > 0) ? cport - 1 : 0;
        }
        // 提取URI信息
        else if (cSpace < 2 && (cSlash > 2 || (i > 0 && str[i-1] == ' ' && str[i] == '/')))
        {
            while (str[i+curi] != ' ')
                *(pstore->uri+curi) = str[i+curi], ++curi;
            i += (curi > 0) ? curi - 1 : 0;
        }
        // 提取版本信息
        else if (cSpace > 1 && cSpace < 3 && str[i] != ' ' && str[i] != '\r' && str[i] != '\n' && str[i] != '\0')
            *(pstore->ver+cver++) = str[i];
    }
    // 请求行格式错误则返回-1
    if (cSpace != 2 || cSlash < 1)
        return -1;
    memset(pstore->ver, 0, sizeof(pstore->ver));
    strcpy(pstore->ver, "HTTP/1.0");
    return 0;
}

// 提取请求报头名
static int getHeadername(char *header, char *usrstr)
{
    int i;
    for (i = 0; header[i] != ':' && header[i] != '\r' && header[i] != '\n'; ++i)
        usrstr[i] = header[i];
    if (!usrstr[0])
        return -1;
    return 0;
}

// 提取Host报头信息，其中的IP和port
static void gethostHeaderInfo(char *hostHdr, requestLSet *prlset)
{
    int i, cSpace, cColons, chost, cport;
    char tempPort[6] = "", tempAddr[30] = "";
    for (i = cSpace = cColons = chost = cport = 0; hostHdr[i] != '\0' && hostHdr[i] != '\r' && hostHdr[i] != '\n'; ++i)
    {
        cSpace += (hostHdr[i] == ' ') ? 1 : 0;
        cColons += (hostHdr[i] == ':') ? 1 : 0;
        if (cSpace > 0 && cColons < 2 && hostHdr[i] != ' ')
            tempAddr[chost++] = hostHdr[i];
        else if (cColons > 1 && hostHdr[i] != ':')
            tempPort[cport++] = hostHdr[i];
    }
    if (tempPort[0])
    {
        memset(prlset->port, 0, sizeof(prlset->port));
        strcpy(prlset->port, tempPort);
    }
    if (tempAddr[0])
    {
        memset(prlset->hostname, 0, sizeof(prlset->hostname));
        strcpy(prlset->hostname, tempAddr);
    }
    if (!prlset->port[0])
        strcpy(prlset->port, "80");
}

// 请求转发，转发给指定的服务器，并将该服务器的响应信息以及大小保存在respondMsg和dataSize中
static void forwardingMsg(requestLSet *prlset, char *forwardMsg, int fmlen, char *respondMsg, int rmlen, int *dataSize)
{
    int clientfd;
    rio_buff clientb;
    clientfd = open_clientfd(prlset->hostname, prlset->port);
    rio_initBuff(&clientb, clientfd);
    rio_writen(clientfd, forwardMsg, fmlen);
    *dataSize = rio_readnb(&clientb, respondMsg, rmlen);
}

// 代理请求处理
int addressRequest(int conntfd)
{
    rio_buff fdBuff;
    rio_initBuff(&fdBuff, conntfd);
    requestLSet rlset;
    initRequeseLine(&rlset);
    char str[200] = "", forwardStr[1000] = "", rqstLine[100] = "", tempstr[30] = "", hostHder[30] = "", *pgetmsg, *dataInfo[3] = {0}, **pdataInfo = (char**)&dataInfo;
    int rlen, cread = 0, dataLen, hitCookie = 0;
    // 接收并处理代理的请求信息
    while ((rlen = rio_readlineb(&fdBuff, str, sizeof(str))) > 0)
    {
        ++cread;
        // 分析提取请求行
        if (cread == 1)
        {
            if (!parse_string(str, &rlset))
            {
                sprintf(rqstLine, "%s %s %s\r\n", rlset.method, rlset.uri, rlset.ver);
                strcat(forwardStr, rqstLine);
            }
            else
            {
                printf("the format of requestLine error!\n");
                return -1;
            }
        }
        else
        {
            // 分析提取Host报头，并检查代理缓冲区是否已储存过对应的响应信息
            // 并添加所需的请求报头
            if (cread == 2)
            {
                if (!(getHeadername(str, tempstr) || strcmp("Host", tempstr)))
                    gethostHeaderInfo(str, &rlset), strcat(forwardStr, str);
                else if (rlset.hostname[0] && rlset.method[0] && rlset.uri[0])
                {
                    if (!rlset.port[0])
                        strcpy(rlset.port, "80");
                    sprintf(hostHder, "Host: %s:%s\r\n", rlset.hostname, rlset.port);
                    strcat(forwardStr, hostHder);
                }
                else
                {
                    printf("lack several values of hostname, IP address, method and URI in HTTP request!\n");
                    return -1;
                }
                printf("%s", forwardStr);
                dataInfo[0] = rlset.method, dataInfo[1] = rlset.hostname, dataInfo[2] = rlset.uri;
                if ((hitCookie = searchDataInCookie(&proxyCookies, pdataInfo, &pgetmsg, &dataLen) + 1) || !strcmp("\r\n", str))
                {
                    strcat(forwardStr, str);
                    break;
                }
                strcat(forwardStr, "Connection: close\r\n");
                strcat(forwardStr, "Proxy-Connection: close\r\n");
                strcat(forwardStr, user_agent_hdr);
            }
            // 将其他的代理请求报头直接添加到请求中
            else if (!getHeadername(str, tempstr) && strcmp("Host", tempstr) && strcmp("Connection", tempstr) && strcmp("Proxy-Connection", tempstr) && strcmp("User-Agent", tempstr))
                strcat(forwardStr, str);
            // 接收到结束行，断开信号接收
            else if (!strcmp("\r\n", str))
            {
                strcat(forwardStr, str);
                break;
            }
        }
        memset(str, 0, sizeof(str));
        memset(tempstr, 0, sizeof(tempstr));
    }
    // 接收代理请求错误处理
    if (rlen == -1)
        perror("read");
    else if (rlen == 0)
        printf("the client has disconnected!\n");
    // 没有已缓存的响应信息的操作
    if (!hitCookie)
    {
        // 消息转发并接收响应
        pgetmsg = (char*)malloc(MAX_CACHE_SIZE);
        forwardingMsg(&rlset, forwardStr, sizeof(forwardStr), pgetmsg, MAX_CACHE_SIZE, &dataLen);
        // 储存响应信息
        if (dataLen <= MAX_OBJECT_SIZE)
            addDataToCookie(&proxyCookies, pgetmsg, dataLen, pdataInfo);
    }
    // 将响应信息返回
    rio_writen(conntfd, pgetmsg, dataLen);
    shutdown(conntfd, SHUT_RDWR);
    if (!hitCookie)
        free(pgetmsg);
    return 0;
}

#endif
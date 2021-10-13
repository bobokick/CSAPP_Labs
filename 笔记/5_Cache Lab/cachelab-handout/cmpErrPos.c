#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int counts = 1;

void initiateCharArray(char *arr, int len)
{
    for (int i = 0; i < len; ++i)
        arr[i] = '\0';
}

void stringAddress(char str[], int len)
{
    int lastPos = len-1, endPrezero = 2, startRe = 2;
    for (; lastPos >= 0 && (str[lastPos] < 'a' || str[lastPos] > 'z'); --lastPos)
        str[lastPos] = '\0';
    for (; endPrezero < len && str[endPrezero] == '0'; ++endPrezero);
    if (endPrezero != 2)
    {
        for (; endPrezero <= lastPos; ++startRe, ++endPrezero)
            str[startRe] = str[endPrezero];
        while (startRe < endPrezero)
            str[startRe++] = '\0';
    }
}

// 比较并发现不同行的位置
int main(int argc, char *args[])
{
    FILE *file1 = fopen(args[1], "r"), *file2 = fopen(args[2], "r");
    char strs1[30] = "", strs2[30] = "";
    for (int i = 0; fgets(strs1, 30, file1) && fgets(strs2, 30, file2); ++i, ++counts)
    {
        stringAddress(strs1, 30);
        stringAddress(strs2, 30);
        if (strcmp(strs1, strs2))
            printf("%d\n", i+1);
        initiateCharArray(strs1, 30);
        initiateCharArray(strs2, 30);
    }
    return 0;
}
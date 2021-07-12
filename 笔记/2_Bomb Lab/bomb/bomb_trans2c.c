#include <stdio.h>
#include <stdlib.h>

void explode_bomb()
{
    printf("BOOM!!!\n"); 
    printf("The bomb has blown up.\n"); 
    exit(1);
}

int string_length(char *str)
{
    // 当字符串为空时返回0
    if (*str == '\0') 
        return 0;
    // 当字符串不为空时，返回该字符串的字符数量(不包括空字符)
    else
    {
        char *inc = str;
        do
            inc += 1;
        while (*inc != '\0');
        return inc - str;
    }
}

int strings_not_equal(char *str, char* code_str)
{
    // 计算两个字符串的长度
    int decode_str_len = string_length(str);
    int code_str_len= string_length(code_str);
    // 长度不相等返回1
    if (decode_str_len != code_str_len) 
        return 1;
    // 密码为空时返回0
    if (*str == '\0') 
        return 0;
    // 两个字符串必须要相等才返回0，否则返回1
    else if (*str == *code_str)
    {
        char *temp_dec = str;
        char *temp_cod = code_str;
        do
        {
            temp_dec += 1;
            temp_cod += 1;
        }
        while (*temp_dec != '\0' && *temp_dec == *temp_cod);
        if (*temp_dec == '\0') 
            return 0;
        else 
            return 1;
    }
    else 
        return 1;
}

void phase_1(char *str)
{
    // 真正的密码
    // *0x402400
    char codes[] = "Border relations with Canada have never been better.";
    // 密码不相等就触发炸弹
    if (strings_not_equal(str, codes))
        explode_bomb();
}

void read_six_numbers(char* str, int *top) 
{
    // 储存int指针的数组
    int *int_ptrs[3];
    // 传入的int指针，指向int数组的第一个元素。
    // 第1个写入
    int *rd = top;
    // 第2个写入
    int *rc = top + 1;
    // 第6个写入
    int_ptrs[1] = top + 5;
    // 第5个写入
    int_ptrs[0] = top + 4;
    // 第4个写入
    int *r9 = top + 3;
    // 第3个写入
    int *r8 = top + 2;
    // 格式字符串
    // *0x4025c3
    char* formats = "%d %d %d %d %d %d"; 
    // 当读取的匹配格式的字符数量小于6时，触发炸弹
    if (sscanf(str, formats, rd, rc, r8, r9, int_ptrs[0], int_ptrs[1]) <= 5)
        explode_bomb();
}

void phase_2(char *str)
{
    // 储存所读取的值
    int int_vals[10];
    int *top = int_vals;
    // 进行读取
    read_six_numbers(str, top);
    // 第一个读取的值必须为1，且后一个读取的值必须
    // 为前一个值的2倍，否则触发炸弹
    // 也就是首项为1，公比为1/2的等比数列
    if (*top == 1)
    {
        int* iter_ptr = top + 1;
        do
            if (*iter_ptr == *(iter_ptr - 1)*2)
                iter_ptr += 1;
            else
                explode_bomb();
        while (iter_ptr != top + 6);
    }
    else explode_bomb();
}

void phase_3(char *str)
{
    // 储存标签值以及目标值
    int int_vals[6];
    // 指向标签值的指针
    // 第2个写入
    int *val = &int_vals[3];
    // 指向目标值的指针
    // 第1个写入
    int *sel = &int_vals[2];
    // 读取的匹配格式的字符数量必须要大于1，标签值要小于8，
    // 且目标值要与对应标签中的值相同，否则触发炸弹
    // *0x4025cf
    if (sscanf(str, "%d %d", sel, val) > 1 && *sel <= 7)
    {
        int res;
        switch (*sel)
        {
            case 0:
                res = 207;
                break;
            case 2:
                res = 707;
                break;
            case 3:
                res = 256;
                break;
            case 4:
                res = 389;
                break;
            case 5:
                res = 206;
                break;
            case 6:
                res = 682;
                break;
            case 7:
                res = 327;
                break;
            default:
                res = 0;
                break;
            case 1:
                res = 311;
                break;
        }
        if (res != *val) explode_bomb();
    }
    else explode_bomb();
}

int func4(int edi, int esi, int edx)
{
    // 第二和第三个形参的某种加减移位等操作
    // 当esi为0，edx为14时，结果值为7
    int res = edx - esi;
    res = (res + ((unsigned)res >> 31)) >> 1;
    int val = res + esi;
    // 相等返回0，否则进行递归调用判断
    if (val == edi)
        return 0;
    else if (val < edi)
        return func4(edi, val + 1, edx) * 2 + 1;
    else 
        return func4(edi, esi, val - 1) * 2;
}

void phase_4(char *str)
{
    // 储存值
    int int_vals[6];
    // 第2个写入
    int *rcx = &int_vals[3];
    // 第1个写入
    int *rdx = &int_vals[2];
    // 我们的输入值必须满足：
    // 读取的匹配格式的字符数量必须要等于2
    // 第1个写入的值要小于等于14且要使func4返回0
    // 第2个写入的值要等于0
    if (sscanf(str, "%d %d", rdx, rcx) != 2 || *rdx > 14 || func4(*rdx, 0, 14) != 0 || *rcx != 0)
        explode_bomb();
}

void phase_5(char *str)
{
    // 我们输入的密码长度必须为6(不包含空字符)，否则触发炸弹
    // 我们输入的字符串会被分成单独的字符，
    // 且每个字符只保留最低的4位，并将其转换成数值，用于表示函数所给定的字符集合的索引
    // 我们要通过给定的字符集合来组合成一个和所给字符串相同的字符串，否则触发炸弹
    if (string_length(str) == 6)
    {
        // 计数
        int count = 0;
        char decode[7];
        // 所给的字符集合
        // *0x4024b0
        // 所需的所有字符索引可以为以下这种组合：
        // 9 15 14 5 6 7
        char char_set[] = "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"; 
        // 给定的字符串，要与其相同
        // *0x40245e
        char code[] = "flyers";
        // 逐位组合成字符串
        do
        {
            int val = (int)(*(str+count) & 0x0f);
            decode[count] = char_set[val];
        }
        while (++count != 6);
        decode[6] = '\0';
        // 比较是否相同
        if (strings_not_equal(decode, code))
            explode_bomb();
    }
    else
        explode_bomb();
}

void phase_6(char *str)
{
    // 储存函数所给的链表的节点的地址
    int* ptrs[6];
    // 储存读取的整数
    int int_vals[8];
    int* vals_top = int_vals;
    int** ptrs_top = ptrs;
    // 进行6个整数的读取
    read_six_numbers(str, vals_top);

    // 我们所读取的所有整数都要满足以下条件，否则触发炸弹
    // 所有的值要大于0，小于7
    // 所有的整数之间不能有相同的
    int* num_iter = vals_top;
    int count = 0;
    do
    {
        // 检查值是否大于0，小于7
        if (*num_iter - 1 <= 5)
        {
            // 逐个检查每个整数
            if (++count != 6)
            {
                int inner_count = count;
                // 检查整数之间是否有重复的
                do
                    if (*num_iter == *(vals_top + inner_count))
                        explode_bomb();
                while (++inner_count <= 5);
                num_iter += 1;
            }
        }
        else
            explode_bomb();
    }
    while (count != 6);

    // 每个整数的值都会变成其值减7的相反数的值
    int *num_iter2 = vals_top;
    do
        *num_iter2 = 7 - *num_iter2;
    while (++num_iter2 != vals_top + 6);

    // 根据该函数所给的链表地址和之前整数的值
    // 对链表的某些节点地址进行储存
    // 储存6个节点的地址
    int *iter_num = vals_top;
    // 该函数所给的链表起始地址，也就是首节点的地址
    int spec_val = 0x6032d0;
    int count2 = 0;
    // 根据之前的整数值，选择所要储存的节点地址
    // 整数值为1则储存第一个节点的地址，2为第二个，以此类推
    while (*iter_num <= 1 || count2 != 6)
    {
        // 根据整数值选择节点地址
        if (*iter_num > 1)
        {
            spec_val = *(int*)(0x6032d0+8);
            for (int count = 2; count != *iter_num; ++count)
                spec_val = *(int*)(spec_val+8);
        }
        else
            spec_val = 0x6032d0;
        // 从指针数组的开头到末尾的顺序逐个储存节点地址
        *(ptrs_top + count2) = (int*)spec_val;
        // 当指针数组全都储存完毕后，退出储存
        if (++count2 == 6)
            break;
        // 逐个选择整数值
        iter_num = vals_top + count2;
    }

    // 使指针数组里所储存的各个节点按从开头到末尾的顺序连接起来，形成一个新的链表。
    int* rsp_32 = *ptrs_top;
    int** iter_ptr = ptrs_top + 1;
    int* temp_ptr = rsp_32;
    do
    {
        // 使数组的前一个元素指向数组的后一个元素。
        *(temp_ptr + 1) = (int)*iter_ptr;
        temp_ptr = *iter_ptr;
        iter_ptr += 1;
    }
    while (iter_ptr != ptrs_top + 6);
    // 形成尾节点
    *iter_ptr = 0;

    // 从首节点向尾节点进行检查，前一节点的值必须要大于或等于后一节点的值
    // 也就是节点值按降序排列。
    int count3 = 5;
    do
    {
        if (*rsp_32 < *((int*)*(rsp_32 + 1)))
            explode_bomb();
        rsp_32 = (int*)*(rsp_32 + 1);
    }
    while (--count3 != 0);
}

// 该函数是用来查找所给的整数值是否存在于所给的二叉排序树中
// 并返回某种位置编号
int fun7(int* root, int val)
{
    // 当树为空时，返回-1
    if (root == NULL) 
        return -1;
    // 当val等于当前节点的值时，返回0
    // 当val大于当前节点的值时，进入右子节点继续查找
    else if (*root <= val)
    {
        if (*root == val)
            return 0;
        else
            return fun7((int*)*(root + 2), val) * 2 + 1;
    }
    // 当val小于当前节点的值时，进入左子节点继续查找
    else
        return fun7((int*)*(root + 1), val) * 2;
}

void secret_phase(char *str)
{
    // 该函数将我们输入的字符串转换成整数
    // 该整数必须小于等于1001，且该整数值必须要使fun7返回2，否则触发炸弹
    // 0x6030f0是所给的二叉排序树的根节点的地址
    unsigned int val = strtol(str, NULL, 10);
    if (val - 1 > 1000 || fun7((int*)0x6030f0, val) != 2)
        explode_bomb();
}

/*
Border relations with Canada have never been better.
1 2 4 8 16 32
0 207
7 0 DrEvil
IONEFG
4 3 2 1 6 5
22
*/
int main()
{
    return 0;
}



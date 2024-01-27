#ifndef CHARUTILS_H
#define CHARUTILS_H

#include <QString>

// 校验和
// len表示作为累加的bytes数，计算完后会在arr[len]直接写入校验和
char checksum(char arr[], int len=8);

// 将 num 写入 arr 的后续 len 个 bytes
void int2char(char arr[], int num, int len=4);

// 将 arr 后续的 n 个 bytes 转为 int 并返回
int32_t char2int32(char arr[]);
uint16_t char2uint16(char arr[]);

// 将 arr 转化为 QString
QString uchar2str(const unsigned char *arr, int len=9);

QString QByteArray2hex(QByteArray buf);

// C89标准规定，short和char会被自动提升为int（整形化，类似地，float也会自动提升为double）
// 这样做是为了便于编译器进行优化，使变量的长度尽可能一样，尽可能提升所产生代码的效率
// char的值当它是正数的时候也同样进行了符号扩展的，只不过正数是前面加0，用%02x打印的时候那些0被忽略；
// 而补码表示的负数的符号扩展却是前面加1，用%02x打印的时候那些1不能被忽略，因此才按照本来的长度输出来。
// 如没有添加unsigned，则当char>0x7F时（如0X80），格式转换为FFFFFF80！
// 解决办法：在char 前面加上 unsigned.
QString char2str(const char *arr, int len=9);

unsigned short calculateCRC(const unsigned char *data, int len);

#endif // CHARUTILS_H

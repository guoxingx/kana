#ifndef MEASURE_H
#define MEASURE_H


// 单次拍摄信息
// 包含：
// 1. 一张图片
// 2. 相对应的相机时间
typedef struct _MEASURE_IMAGE {
    char* filepath;
    int   timestamp;
} MImage;


/*
char   变量，单个字符

char[] 变量，字符串，指向第一个字符的地址
    char[] a = "hello";

char*  变量，字符串，指向第一个字符的地址。
    但是创建的char[]是常量
    const char* a = "hello";
    改变 a 的值只是把 a 指向另一个常量（字符串首个字符的地址），并不改变底下的常量 "hello"
*/


// 单点位置的测量信息
// 包含：
// 1. 一个位置
// 2. 位置（平移台）的时间信息
// 3. 多个单次拍摄信息
typedef struct _MEASURE_LOCATION {
    int    location;
    int    timestamp;
    MImage files;
} MLocation;


class Measure
{
public:
    Measure();

    int set_positon();
};

#endif // MEASURE_H

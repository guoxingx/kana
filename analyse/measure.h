#ifndef MEASURE_H
#define MEASURE_H

#include <string>

/*
char   变量，单个字符

char[] 变量，字符串，指向第一个字符的地址
    char[] a = "hello";

char*  变量，字符串，指向第一个字符的地址。
    但是创建的char[]是常量
    const char* a = "hello";
    改变 a 的值只是把 a 指向另一个常量（字符串首个字符的地址），并不改变底下的常量 "hello"
*/

class Measure
{
public:
    Measure();
    ~Measure();

    // 计算束宽，返回是错误码，束宽更新到Meaure.beamwidth
    int cal_beamwidth();
    int test();

    // 本次测量的图片路径
    std::string imagepath;
    // z轴的位置
    double position;
    // 束宽
    double beamwidth;
    // 时间戳
    int ts_pos;
    int ts_snap;

    // 多点拟合法测M2因子
    static int fitcurve(float a[3], float z[], float w[]);
};

#endif // MEASURE_H

#ifndef TYPES_H
#define TYPES_H

// types.h 会被多次引用
// #ifndef 可以保证该文件不会被include多次
//
// 在使用Sensor_Info的时候，一般还是要用 include "types.h" 而不是 class Sensor_Info;
// 如果用 前向声明 (forward declaration)
// 由于前向声明而没有定义的类是不完整的，所以class A只能用于定义指针、引用、或者用于函数形参的指针和引用，
// 不能用来定义对象，或访问类的成员。

// 探测器信息
//
// 统一多种厂家的数据类型
typedef struct _SENSOR_INFO{
    int     width;
    int     height;
    int     bufSize;
    int     bufSizeMax;
}Sensor_Info;

#endif // TYPES_H

/*

在C中定义一个结构体类型要用 typedef:
typedef struct Student
{
    int a;
}Stu;  // 这里的 Stu 实际上就是 struct Student 的别名。

声明变量的时候就可：Stu s1;
如果没有 typedef 就必须用 struct Student s1; 来声明变量

另外这里也可以不写 Student
typedef struct
{
    int a;
}Stu;
于是也不能 struct Student stu1;
声明变量用 Stu s1


但在c++里很简单，直接
struct Student
{
    int a;
};

于是就定义了结构体类型 Student，声明变量时直接 Student s2；

===========================================

其次：
在c++中如果用 typedef 的话，又会造成区别：

// 不用 typedef 的例子：
struct Student
{
    int a;
}s1;
s1是一个变量，使用时可以直接访问s1.a

// 用 typedef 的例子：
typedef struct Student2
{
    int a;
}Stu2;
Stu2是一个结构体类型
必须先 stu2 s2; 然后 s2.a=10;

*/

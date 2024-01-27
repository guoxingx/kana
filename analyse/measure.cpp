#include "measure.h"

#include "opencv2/opencv.hpp"

using namespace cv;

Measure::Measure() {}

Measure::~Measure() {}

int Measure::test() {
    String imagepath = "D:/Developer/Projects/kana/statics/test/001.tif";
//    String imagepath = "C:/Users/wuyi/Desktop/016.png";
    Mat image = imread(imagepath, IMREAD_GRAYSCALE);
    if (image.empty())
        return -2;

    // 去除噪声，将小于一定灰度值的像素点视为噪声，直接置为0
    // 防止其对寻找轮廓函数findContours的影响
    threshold(image, image, 32, 255, THRESH_TOZERO);

    // 轮廓坐标点的二维向量，因为findContours函数会返回多组轮廓向量
    std::vector <std::vector<Point>> contours;
    // RETR_TREE:         找所有轮廓，用内外包含的序列关系
    // RETR_EXTERNAL:     找外部轮廓
    // CHAIN_APPROX_NONE: 不适用近似，保存边界上所有点
    findContours(image, contours, RETR_TREE, CHAIN_APPROX_NONE);
    // 拟合的椭圆曲线向量，尺寸与contours保持一致
    std::vector <RotatedRect> ellipse_contours(contours.size());

    // 记录最大区域及其下标
    int max_index = 0;
    float max_area = 0;

    // 拟合椭圆轮廓
    int j = 0;
    for (int i = 0; i < contours.size(); i++) {
        // 10个点以下的轮廓视为无效
        if (contours[i].size() > 10) {
            // 拟合椭圆曲线，返回一个描述椭圆的旋转矩形（与该矩形外切）
            ellipse_contours[i] = fitEllipse(Mat(contours[i]));
            Size2f size = ellipse_contours[i].size;
            // 记录最大区域及其位置，区域过小的轮廓视为无效
            if (size.height > 15 && size.width > 15 && size.width * size.height > max_area) {
                max_area = size.width * size.height;
                max_index = i;
            }
            j ++;
        }
    }

    // 大于一定值的区域视为有效，若所有轮廓都尺寸过小，直接返回错误
    if (max_area == 0)
        return -3;

    Mat dest = image.clone();
    // 转成彩色格式，方便看到绘制曲线
    cvtColor(dest, dest, COLOR_GRAY2BGR);
    // 高精度CV_32F
//    dest.convertTo(dest, CV_32FC1);

    // 取最大区域的轮廓，绘制中心标记与轮廓
    Point2d center = ellipse_contours[max_index].center;

    for (int i = 0; i < ellipse_contours.size(); i ++) {
        if (contours[i].size() < 100)
            continue;
        std::cout << "ellipse index: " << i << ", size: " << contours[i].size() << "\n";
//        Scalar color(i * 22 % 255, i * 33 % 255, i * 55 % 255);
        Scalar color(0, 0, 255);
        // 绘制中心一个十字标记
        // line(dest, Point2f(center.x, center.y-16), Point2f(center.x, center.y+16), color, 4);
        // line(dest, Point2f(center.x-16, center.y), Point2f(center.x+16, center.y), color, 4);

        // 绘制椭圆轮廓，
        ellipse(dest, ellipse_contours[i], color, 4);
    }

    // 需要加拓展名，否则会报错could not find a writer for the specified extension) in cv::imwrite_
    imwrite("C:/Users/wuyi/Desktop/test.png", dest);

    imshow("test_image", dest);
    // cv::waitKey(0);//任意按键按下，图片显示结束，返回按键键值
    // 等待至少20ms图片显示才结束，期间按下任意键图片显示结束，返回按键键值
    waitKey();
    return 0;
}

// 根据现有图片计算束宽
int Measure::cal_beamwidth() {
    // 没有图片，直接返回
    if (imagepath.empty())
        return -1;
    Mat image = imread(imagepath, IMREAD_GRAYSCALE);
    if (image.empty())
        return -2;

    // 去除噪声，将小于一定灰度值的像素点视为噪声，直接置为0
    // 防止其对寻找轮廓函数findContours的影响
    threshold(image, image, 32, 255, THRESH_TOZERO);

    // 轮廓坐标点的二维向量，因为findContours函数会返回多组轮廓向量
    std::vector <std::vector<Point>> contours;
    // 轮廓坐标点的4 int 向量，这里用不到
    std::vector <Vec4i> hierarcy;
    // RETR_TREE：        找所有轮廓，用内外包含的序列关系
    // CHAIN_APPROX_NONE：不适用近似，保存边界上所有点
    findContours(image, contours, hierarcy, RETR_TREE, CHAIN_APPROX_NONE);
    // 拟合的椭圆曲线向量，尺寸与contours保持一致
    std::vector <RotatedRect> ellipse_contours(contours.size());

    // 记录最大区域及其下标
    int max_index = 0;
    float max_area = 0;

    // 拟合椭圆轮廓
    for (int i = 0; i < contours.size(); i++) {
        // 10个点以下的轮廓视为无效
        if (contours[i].size() >= 10) {
            // 拟合椭圆曲线，返回一个描述椭圆的旋转矩形（与该矩形外切）
            ellipse_contours[i] = fitEllipse(cv::Mat(contours[i]));
            Size2f size = ellipse_contours[i].size;
            // 记录最大区域及其位置，区域过小的轮廓视为无效
            if (size.height > 15 && size.width > 15 && size.width * size.height > max_area) {
                max_area = size.width * size.height;
                max_index = i;
            }
        }
    }

    // 大于一定值的区域视为有效，若所有轮廓都尺寸过小，直接返回错误
    if (max_area == 0)
        return -3;

    float w1 = ellipse_contours[max_index].size.height;
    float w2 = ellipse_contours[max_index].size.width;

    // 取椭圆较小的为束宽
    this->beamwidth = w1 < w2? w1: w2;
    return 0;

    Mat dest = image.clone();

    // 取最大区域的轮廓，绘制中心标记与轮廓
    Point2d center = ellipse_contours[max_index].center;

    // 绘制中心一个十字标记
    line(dest, Point2f(center.x, center.y-16), Point2f(center.x, center.y+16), COLORMAP_PINK, 4);
    line(dest, Point2f(center.x-16, center.y), Point2f(center.x+16, center.y), COLORMAP_PINK, 4);

    // 绘制椭圆轮廓
    ellipse(dest, ellipse_contours[max_index], COLORMAP_HOT, 4);

    // 需要加拓展名，否则会报错could not find a writer for the specified extension) in cv::imwrite_
    imwrite(imagepath.append("-").append(std::to_string(this->beamwidth).append(".tif")), dest);

    imshow("test_image", dest);
    // cv::waitKey(0);//任意按键按下，图片显示结束，返回按键键值
    // 等待至少20ms图片显示才结束，期间按下任意键图片显示结束，返回按键键值
    cv::waitKey();

    return 0;
}

// 多点拟合法测M2因子
// 参数：
//      a[3]: 系数数组，计算完成后更新
//      z   : 位置z的数组
//      w   : 对应位置束宽w的数组
// 返回：错误码
// Az = w^2
int Measure::fitcurve(float a[3], float z[], float w[]) {
    // 参数的数量
    int n1 = sizeof(*z) / sizeof(z[0]);
    int n2 = sizeof(*w) / sizeof(w[0]);
    if (n1 < 5 || n1 != n2)
        return -1;
    int n = n1;

    // 构建 n行3列 的矩阵A，并带入参数z
    cv::Mat A = cv::Mat::zeros(cv::Size(3, n), CV_64FC1);
    for (int i = 0; i < n; i++) {
        A.at<double>(i, 0) = 1;
        A.at<double>(i, 1) = z[i];
        A.at<double>(i, 2) = z[i] * z[i];
    }

    // 构建 n行1列 的矩阵b，带入参数 w^2
    cv::Mat b = cv::Mat::zeros(cv::Size(1, n), CV_64FC1);
    for (int i = 0; i < n; i++)
        b.at<double>(i, 0) = w[i] * w[i];

    // A.T * A * x = A.T * b
    cv::Mat ATA = A.t() * A;
    cv::Mat ATb = A.t() * b;

    // 拟合曲线
    cv::Mat result = cv::Mat::zeros(cv::Size(1, 3), CV_64FC1);
    // cv::solve 线性/最小二乘法 问题求解
    // CV_EXPORTS_W bool solve(InputArray src1, InputArray src2, OutputArray dst, int flags = DECOMP_LU);
    //   @param src1 input matrix on the left-hand side of the system.
    //   @param src2 input matrix on the right-hand side of the system.
    //   @param dst output solution.
    //   @param flags solution (matrix inversion) method (#DecompTypes)
    cv::solve(ATA, ATb, result);
    a[0] = result.at<double>(0, 0);
    a[1] = result.at<double>(1, 0);
    a[2] = result.at<double>(2, 0);

    return 0;
}

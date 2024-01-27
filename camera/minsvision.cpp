#include "minsvision.h"

#include "capturethread.h"


//unsigned char        * pRawBuf = NULL;      //raw数据


// minsvision 相机接口
MinsVision::MinsVision()
{
    hCamera = -1;
    pRawBuf = NULL;
    pRgbBuf = NULL;
}

// 初始化相机配置
int MinsVision::init(Sensor_Info* info)
{
    int                     iCameraCounts = 4;
    int                     iStatus=-1;
    tSdkCameraDevInfo       tCameraEnumList[4];

    //sdk初始化  0 English 1 中文
    CameraSdkInit(1);

    //枚举设备，并建立设备列表
    CameraEnumerateDevice(tCameraEnumList, &iCameraCounts);

    //没有连接设备
    if(iCameraCounts == 0){
        return -1;
    }

    //相机初始化。初始化成功后，才能调用任何其他相机相关的操作接口
    iStatus = CameraInit(&tCameraEnumList[0], -1, -1, &hCamera);

    //初始化失败
    if(iStatus != CAMERA_STATUS_SUCCESS){
        return -1;
    }

    //获得相机的特性描述结构体。该结构体中包含了相机可设置的各种参数的范围信息。决定了相关函数的参数
    CameraGetCapability(hCamera, &pCameraInfo);

    // 设置相机信息返回参数
    tSdkImageResolution     *pImageSizeDesc = pCameraInfo.pImageSizeDesc;// 预设分辨率选择
    tSdkImageResolution     sResolution;  //获取当前设置到分辨率
    // 获得当前预览的分辨率
    // 这里的 width height bufSize 是实际数值
    CameraGetImageResolution(hCamera, &sResolution);
    info->width = pImageSizeDesc[sResolution.iIndex].iWidth;
    info->height = pImageSizeDesc[sResolution.iIndex].iHeight;
    info->bufSize = info->width * info->height;
    info->bufSizeMax = bufSizeMax;
    bufSize = info->bufSize;

    // 申请最大可能的内存，用最大的 height * width * 3通道
    // 类型转换 (void*)malloc(size) void*是未确定类型的指针
    bufSizeMax = pCameraInfo.sResolutionRange.iHeightMax * pCameraInfo.sResolutionRange.iWidthMax;
    pRgbBuf = (unsigned char*)malloc(bufSizeMax * 3);

    /*让SDK进入工作模式，开始接收来自相机发送的图像数据。
     * 如果当前相机是触发模式，则需要接收到触发帧以后才会更新图像。*/
    CameraPlay(hCamera);

    /*
        设置图像处理的输出格式，彩色黑白都支持RGB24位
    */
    if(pCameraInfo.sIspCapacity.bMonoSensor){
        CameraSetIspOutFormat(hCamera,CAMERA_MEDIA_TYPE_MONO8);
    }else{
        CameraSetIspOutFormat(hCamera,CAMERA_MEDIA_TYPE_RGB8);
    }

    return 0;
}

// 获取图片数据
int MinsVision::get_image(unsigned char* readBuf)
{
    // CameraGetImageBuffer 获取相机数据
    //  params1：相机句柄；CameraHandle即int；
    //  params2：图像帧头信息；tSdkFrameHead*；
    //  params3: 返回数据bytes；BYTE**；
    //  params4：超时时间；毫秒；UINT；
    //  return： CAMERA_STATUS_XXX；相机状态
    if (CameraGetImageBuffer(hCamera, &tFrameHead, &pRawBuf, 2000) == CAMERA_STATUS_SUCCESS)
    {
//        qDebug("success: CameraGetImageBuffer()");
        // CameraImageProcess 处理相机数据，如饱和度，色彩，增益，降噪等
        //  params1：相机句柄；CameraHandle即int；
        //  params2：输入数据bytes；BYTE**；
        //  params3: 返回数据bytes；BYTE**；
        //  params4：图像帧头信息；tSdkFrameHead*；
        //  return： CAMERA_STATUS_XXX；相机状态；
        CameraImageProcess(hCamera, pRawBuf, pRgbBuf, &tFrameHead);

        // 释放缓存: raw
        CameraReleaseImageBuffer(hCamera, pRawBuf);

        // 用句柄判断照片类型
        if (tFrameHead.uiMediaType==CAMERA_MEDIA_TYPE_MONO8) {
            // 单色图
            // 从rgbBuffer拷贝到readBuf，大小从WHInfo获取
            memcpy(readBuf, pRgbBuf, bufSize);
        } else {
            // 彩色图
            // 从rgbBuffer拷贝到readBuf，大小从WHInfo获取
            memcpy(&readBuf, pRgbBuf, bufSize*3);
        }

        return 0;
    } else {
        qDebug("failed: CameraGetImageBuffer()");
        return -1;
    }
}

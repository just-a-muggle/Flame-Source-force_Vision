#include <iostream>
#include <MvCameraControl.h>
#include <opencv2/opencv.hpp>
#include <memory>

class HikCamera {
public:
    HikCamera();
    ~HikCamera();

    bool Initialize(int cameraIndex);
    void StartCapture();
    void StopCapture();

private:
    static void* m_Handle;                // 相机句柄
    int m_Width = 1920;                   // 默认分辨率宽度
    int m_Height = 1080;                  // 默认分辨率高度
    bool m_IsCapturing = false;           // 是否正在捕获
    unsigned char* m_Buffer = nullptr;    // 图像缓存

    static void PrintDeviceList(const MV_CC_DEVICE_INFO_LIST& deviceList);
};

void* HikCamera::m_Handle = nullptr;

HikCamera::HikCamera() = default;

HikCamera::~HikCamera() {
    if (m_IsCapturing) {
        StopCapture();
    }
    if (m_Handle) {
        MV_CC_DestroyHandle(m_Handle);
        m_Handle = nullptr;
    }
    if (m_Buffer) {
        delete[] m_Buffer;
        m_Buffer = nullptr;
    }
}

bool HikCamera::Initialize(int cameraIndex) {
    // 枚举设备
    MV_CC_DEVICE_INFO_LIST deviceList = {0};
    if (MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &deviceList) != MV_OK) {
        std::cerr << "Failed to enumerate devices!" << std::endl;
        return false;
    }
    PrintDeviceList(deviceList);

    if (cameraIndex >= deviceList.nDeviceNum) {
        std::cerr << "Invalid camera index!" << std::endl;
        return false;
    }

    // 创建设备句柄
    if (MV_CC_CreateHandle(&m_Handle, deviceList.pDeviceInfo[cameraIndex]) != MV_OK) {
        std::cerr << "Failed to create camera handle!" << std::endl;
        return false;
    }

    // 打开设备
    if (MV_CC_OpenDevice(m_Handle) != MV_OK) {
        std::cerr << "Failed to open camera device!" << std::endl;
        return false;
    }

    // 获取图片宽高
    MVCC_INTVALUE stIntValue = {0};
    MV_CC_GetIntValue(m_Handle, "Width", &stIntValue);
    m_Width = stIntValue.nCurValue;
    MV_CC_GetIntValue(m_Handle, "Height", &stIntValue);
    m_Height = stIntValue.nCurValue;

    // 设置连续采集模式
    if (MV_CC_SetEnumValue(m_Handle, "TriggerMode", MV_TRIGGER_MODE_OFF) != MV_OK) {
        std::cerr << "Failed to set trigger mode!" << std::endl;
        return false;
    }

    // 分配缓存空间
    m_Buffer = new unsigned char[m_Width * m_Height * 3];

    return true;
}

void HikCamera::StartCapture() {
    if (MV_CC_StartGrabbing(m_Handle) != MV_OK) {
        std::cerr << "Failed to start grabbing!" << std::endl;
        return;
    }
    m_IsCapturing = true;

    while (m_IsCapturing) {
        MV_FRAME_OUT stFrameOut = {0};
        if (MV_CC_GetImageBuffer(m_Handle, &stFrameOut, 1000) == MV_OK) {
            memcpy(m_Buffer, stFrameOut.pBufAddr, stFrameOut.stFrameInfo.nFrameLen);

            // 将缓存转换为 OpenCV 图像
            cv::Mat image(m_Height, m_Width, CV_8UC3, m_Buffer);
            cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

            cv::namedWindow("HikCamera", cv::WINDOW_FREERATIO);
            // 显示图像
            cv::imshow("HikCamera", image);

            // 按下 'q' 键退出
            if (cv::waitKey(1) == 'q') {
                StopCapture();
            }

            MV_CC_FreeImageBuffer(m_Handle, &stFrameOut);
        }
    }
}

void HikCamera::StopCapture() {
    if (m_IsCapturing) {
        MV_CC_StopGrabbing(m_Handle);
        m_IsCapturing = false;
    }
}

void HikCamera::PrintDeviceList(const MV_CC_DEVICE_INFO_LIST& deviceList) {
    std::cout << "Available devices: " << std::endl;
    for (unsigned int i = 0; i < deviceList.nDeviceNum; ++i) {
        MV_CC_DEVICE_INFO* deviceInfo = deviceList.pDeviceInfo[i];
        if (deviceInfo->nTLayerType == MV_GIGE_DEVICE) {
            std::cout << "Device [" << i << "] GigE, IP: "
                      << deviceInfo->SpecialInfo.stGigEInfo.nCurrentIp << std::endl;
        } else if (deviceInfo->nTLayerType == MV_USB_DEVICE) {
            std::cout << "Device [" << i << "] USB" << std::endl;
        }
    }
}

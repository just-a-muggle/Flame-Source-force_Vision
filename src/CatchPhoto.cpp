#include "FSF_Capture.hpp"

int main() {
    HikCamera camera;

    // 初始化相机
    if (!camera.Initialize(0)) {
        return -1;
    }

    // 开始捕获
    camera.StartCapture();

    return 0;
}

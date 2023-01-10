#include <QCoreApplication>
#include <QTimer>
#include <vector>
#include <string>
#include "backend.h"
#include "utils/config.h"

int main(int argc, char* argv[])
{   
    QCoreApplication app(argc, argv);
    // 初始化backend
    auto& config = eegneo::utils::ConfigLoader::instance();
    std::size_t channelNum = config.get<std::vector<std::string>>("Acquisition", "Electrodes").size();
    std::size_t sampleFreqHz = config.get<std::size_t>("Acquisition", "SampleFrequencyHz");
    eegneo::AcquisitionBackend backend{channelNum, sampleFreqHz};
    // 启动backend在主线程中的任务，进入事件循环
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&backend]{ backend.doTaskInMainThread(); });
    timer.start(0);
    return app.exec();
}

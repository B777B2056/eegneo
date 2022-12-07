#include <QCoreApplication>
#include <string>
#include "sampler.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    eegneo::TestDataSampler sampler{std::stoull(argv[1])};
    sampler.start();
    return app.exec();
}

#include <QtCharts>
#include <QQueue>
#include <QString>

/*全局变量(用于主线程与子线程间传递信息)*/
int maxVoltage;
int channel_num;
int threshold;  // 图上最多显示多少个数据点
bool isFilt;  // 是否进行滤波
std::vector<double> originalData;  // 通道的原始数据
std::vector<double> filtData;  // 滤波后输入图像的数据
std::vector<QChart *> charts;
std::vector<QSplineSeries *> series;
std::vector<QQueue<QPointF>> pointQueue;
std::map<QLineSeries *, std::pair<qint64, QGraphicsSimpleTextItem *>> marks;

namespace eegneo
{
    namespace global
    {
        QString GSubjectNumber;
        
    }   // namespace global
}   // namespace eegneo

#include "preprocesswindow.h"
#include "ui_preprocesswindow.h"
#include "ui_charthelp.h"

PreprocessWindow::PreprocessWindow(QString tempFile, QWidget *parent) :
    QMainWindow(parent),
    channelNum(1), maxVotagle(50), isOverlapping(false),
    ui(new Ui::PreprocessWindow)
{
    ui->setupUi(this);
    /*初始化缓存文件名称*/
    this->tempFile = tempFile;
    /*链接信号与槽*/
    connect(this, SIGNAL(returnMain()), parent, SLOT(goToMainWindow()));
    connect(ui->actionlocalFile, SIGNAL(triggered()), this, SLOT(readDataFromLocal()));
//    connect(ui->actionedf, SIGNAL(triggered()), this, SLOT(readEDFPlus()));
//    connect(ui->actionbdf, SIGNAL(triggered()), this, SLOT(readBDF()));
//    connect(ui->actioncnt, SIGNAL(triggered()), this, SLOT(readCNT()));
}

PreprocessWindow::~PreprocessWindow()
{
    delete ui;
}

void PreprocessWindow::on_pushButton_clicked()
{
    emit returnMain();
}

/*绘图初始化*/
void PreprocessWindow::initChart(int channelNum, int allTimes)
{
    /*初始化*/
    help = new ChartHelp(this);
    axisX = new QValueAxis;
    axisY = new QCategoryAxis;
    axisMark = new QCategoryAxis;
    chart = new QChart;
    for(int i = 0; i < channelNum; i++)
    {
        series.push_back(new QSplineSeries);
    }
    //设置x轴
    axisX->setRange(0, allTimes);
    axisX->setTickCount(allTimes);
    chart->addAxis(axisX, Qt::AlignBottom);
    //设置Mark辅助坐标轴
    axisMark->setRange(0, allTimes);
    axisMark->setStartValue(0);
    axisMark->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    chart->addAxis(axisMark, Qt::AlignTop);
    //设置y轴
    axisY->setMin(0);
    axisY->setMax(channelNum * 50 * 2);
    axisY->setStartValue(0);
    axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    chart->addAxis(axisY, Qt::AlignLeft);
    for(int index = 0; index < channelNum; index++)
    {
        //链接数据
        series[index]->setUseOpenGL(true);
        QPen splinePen;
        splinePen.setBrush(Qt::blue);
        splinePen.setColor(Qt::blue);
        series[index]->setPen(splinePen);
        chart->addSeries(series[index]);
        chart->setAxisX(axisX, series[index]);
        chart->setAxisY(axisY, series[index]);
    }
    //设置界面显示
    chart->legend()->hide();
    chart->setTheme(QChart::ChartThemeLight);
    //charts->setMargins({-10, 0, 0, -10});
    chart->axisX()->setGridLineVisible(false);
    chart->layout()->setContentsMargins(0, 0, 0, 0);//设置外边界全部为0
    chart->setMargins(QMargins(0, 0, 0, 0));//设置内边界全部为0
    chart->setBackgroundRoundness(0);//设置背景区域无圆角

    help->ui->widget_pre_wave->setChart(chart);
    QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
    item->setSizeHint(QSize(1500, 600));
    ui->listWidget->setItemWidget(item, help->ui->widget_pre_wave);
}

/*从本程序缓存文件中导入数据*/
void PreprocessWindow::readDataFromLocal()
{
    int allTime = 0, col = 0;
    std::string allEvents = "";
    std::ifstream data_file, event_file;
//    data_file.open(tempFile.toStdString() + "_samples.txt", std::ios::in);
//    event_file.open(tempFile.toStdString() + "_events.txt", std::ios::in);
    data_file.open("samples.txt", std::ios::in);
    /*计算总时间*/
    while(data_file.peek() != EOF)
    {
        std::string str;
        std::getline(data_file, str);
        ++col;
    }
    allTime = col * SAMPLE_RATE / 1000;
    col = 0;
    data_file.close();
    /*绘制数据点*/
    data_file.open("samples.txt", std::ios::in);
//    data_file.open(tempFile.toStdString() + "_samples.txt", std::ios::in);
    while(data_file.peek() != EOF)
    {
        std::string str;
        std::getline(data_file, str);
        if(col)
        {
            std::stringstream ss(str);
            for(int j = 0; j < channelNum; j++)
            {
                double m;
                ss >> m;
                samplePoints[j].push_back(QPointF((double)col * (double)SAMPLE_RATE / 1000.0, m));
            }

        }
        else
        {
            for (std::size_t j = 0; j < str.length(); j++)
            {
                if(str[j] == ' ')
                    channelNum++;
            }
            initChart(channelNum, allTime);  // 初始化绘图板
        }
        ++col;
    }
    data_file.close();
    /*绘制事件*/
    col = 0;
    std::map<std::string, long long> marks;
    event_file.open("events.txt", std::ios::in);
    while(event_file.peek() != EOF)
    {
        std::string str;
        std::getline(event_file, str);
        if(col)
        {
            long long run_time;
            std::string event;
            std::stringstream ss(str);
            ss >> run_time >> event;
            marks[event] = run_time;
        }
        ++col;
    }
    event_file.close();
    std::map<std::string, long long>::iterator iter;
    for(iter = marks.begin(); iter != marks.end(); iter++)
    {
        markColors[iter->first] = getRadomColor(165, 42, 42);
    }
    col = 0;
    event_file.open("events.txt", std::ios::in);
    while(event_file.peek() != EOF)
    {
        std::string str;
        std::getline(event_file, str);
        if(col)
        {
            long long run_time;
            std::string event;
            std::stringstream ss(str);
            ss >> run_time >> event;
            eventLines.push_back(std::make_pair(event, run_time / 10000.0));
        }
        ++col;
    }
    event_file.close();
    /*设置基本信息*/
    ui->label_5->setText(QString::number(channelNum));
    ui->label_7->setText(QString::number(col - 1));
    ui->label_9->setText("1");
    ui->label_11->setText(QString::number(SAMPLE_RATE));
    ui->label_13->setText(QString::number(allTime) + "s");
    /*画图*/
    paintChart();
}

/*绘图*/
void PreprocessWindow::paintChart()
{
    /*设置通道标签*/
    for(int j = channelNum - 1; j >= 0; j--)
        axisY->append(QString::number(j + 1), maxVotagle * (2 * channelNum - 2 * j - 1));
    /*绘制数据点*/
    std::map<int, std::vector<QPointF>>::iterator sample_iter;
    for(sample_iter = samplePoints.begin(); sample_iter != samplePoints.end(); sample_iter++)
    {
        for(unsigned int i = 0; i < sample_iter->second.size(); i++)
            series[sample_iter->first]->append(QPointF(sample_iter->second[i].x(),
                                                       isOverlapping
                                                     ? sample_iter->second[i].y() + channelNum * maxVotagle
                                                     : sample_iter->second[i].y() + maxVotagle * (2 * channelNum - 2 * sample_iter->first - 1)));
    }
    /*绘制事件*/
    for(std::size_t i = 0; i < eventLines.size(); i++)
    {
        /*画直线*/
        QLineSeries *line = new QLineSeries;
        QPen splinePen;
        splinePen.setColor(markColors[eventLines[i].first]);
        line->setPen(splinePen);
        line->append(QPointF(eventLines[i].second, 0));
        line->append(QPointF(eventLines[i].second, channelNum * maxVotagle *2));
        chart->addSeries(line);
        chart->setAxisX(axisX, line);
        chart->setAxisY(axisY, line);
        /*显示文字标记*/
        axisMark->append(QString::fromStdString(eventLines[i].first), eventLines[i].second);
    }
}

/*生成随机颜色用于不同的Mark*/
QColor PreprocessWindow::getRadomColor(int baseR, int baseG, int baseB)
{
    return QColor(baseR + (-10 + rand() % 20), baseG + (-10 + rand() % 20), baseB + (-10 + rand() % 20));
}

void PreprocessWindow::on_comboBox_2_currentIndexChanged(int index)
{
    if(index == 0)
        maxVotagle = 50;
    else if(index == 1)
        maxVotagle = 100;
    else
        maxVotagle = 200;
    for(int j = 0; j < channelNum; j++)
    {
        series[j]->clear();
        axisY->remove(QString::number(j + 1));
    }
    /*设置Y轴范围*/
    chart->axisY()->setMax(8 * maxVotagle * 2);
    /*重新绘图*/
    paintChart();
}

/*查看事件类型*/
void PreprocessWindow::on_pushButton_4_clicked()
{

}

/*波形重叠/取消重叠复用按钮*/
void PreprocessWindow::on_pushButton_5_clicked()
{
    for(int j = 0; j < channelNum; j++)
    {
        series[j]->clear();
        axisY->remove(QString::number(j + 1));
    }
    if(!isOverlapping)
    {
        /*设置按钮文字与标志位*/
        ui->pushButton_5->setText("取消重叠");
        isOverlapping = true;
    }
    else
    {
        /*取消重叠*/
        ui->pushButton_5->setText("波形重叠");
        isOverlapping = false;
    }
    paintChart();
}

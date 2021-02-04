#include "preprocesswindow.h"
#include "ui_preprocesswindow.h"
#include "ui_charthelp.h"

PreprocessWindow::PreprocessWindow(QWidget *parent) :
    QMainWindow(parent),
    interval(1), channelNum(1), maxVotagle(50),
    start(0.0), end(5.0), jmp_start(-1.0), jmp_end(-1.0), allTime(0.0),
    hasOpen(false), isJmp(false), isOverlapping(false),
    channelsName(nullptr),
    ui(new Ui::PreprocessWindow)
{
    ui->setupUi(this);
    /*链接信号与槽*/
    connect(this, SIGNAL(returnMain()), parent, SLOT(goToMainWindow()));
    connect(ui->actionlocalFile, SIGNAL(triggered()), this, SLOT(readDataFromLocal()));
    connect(ui->actionedf, SIGNAL(triggered()), this, SLOT(readEDForBDF()));
//    connect(ui->actioncnt, SIGNAL(triggered()), this, SLOT(readCNT()));
    connect(ui->actionrenameMotage, SIGNAL(triggered()), this, SLOT(setChannelsName()));
}

PreprocessWindow::~PreprocessWindow()
{
    std::size_t i;
    for(i = 0; i < series.size(); i++)
        delete series[i];
    for(i = 0; i < pItems.size(); i++)
        delete pItems[i];
    for(i = 0; i < lines.size(); i++)
        delete lines[i];
    delete axisX;
    delete axisY;
    delete chart;
    delete help;
    delete ui;
}

/*重设通道电极名称*/
void PreprocessWindow::setChannelsName()
{
    if(channelsName)
    {
        /*弹窗口让用户输入各通道名称*/
        SetChannelName scl(channelNum);
        int rec = scl.exec();
        if(rec == QDialog::Accepted)
        {
            for(int i = 0; i < channelNum; i++)
                channelsName[i] = scl.names[i].toStdString();
        }
        /*清空之前的内容*/
        for(int j = 0; j < channelNum; j++)
        {
            axisY->remove(QString::number(j + 1));
        }
        /*设置通道标签*/
        for(int j = channelNum - 1; j >= 0; j--)
            axisY->append(QString::fromStdString(channelsName[j]), maxVotagle * (2 * channelNum - 2 * j - 1));
    }
}

/*从本程序缓存文件中导入数据*/
void PreprocessWindow::readDataFromLocal()
{
    if(!samplePoints.empty())
        samplePoints.clear();
    if(!eventLines.empty())
        eventLines.clear();
    int col = 0;
    QString samplesFile, eventsFile;
    std::string allEvents = "";
    std::ifstream data_file, event_file;
    if(tempFile != "")
    {
        samplesFile = tempFile + "_samples.txt";
        eventsFile = tempFile + "_events.txt";
    }
    QFile file1(samplesFile), file2(eventsFile);
    if (tempFile == "" || !file1.exists() || !file2.exists())
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::critical(this, this->tr("错误"),
                                        "未在默认位置找到目标文件！\n是否手动选择文件？",
                                        QMessageBox::Ok | QMessageBox::No);
        if (reply == QMessageBox::Ok)
        {
            samplesFile = QFileDialog::getOpenFileName(this,
                                                       tr("选择数据文件"),
                                                       "",
                                                       tr("*.txt")); //选择路径
            eventsFile = QFileDialog::getOpenFileName(this,
                                                      tr("选择事件标注文件"),
                                                      "",
                                                      tr("*.txt")); //选择路径
        }
        else
        {
            return;
        }
    }
    data_file.open(samplesFile.toStdString(), std::ios::in);
    /*计算总时间*/
    while(data_file.peek() != EOF)
    {
        std::string str;
        std::getline(data_file, str);
        ++col;
    }
    allTime = (double)col  / (double)SAMPLE_RATE;
    col = 0;
    data_file.close();
    /*读取数据点*/
    data_file.open(samplesFile.toStdString(), std::ios::in);
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
                samplePoints[j].push_back(QPointF((double)col / (double)SAMPLE_RATE, m));
            }
        }
        else
        {
            for (std::size_t j = 0; j < str.length(); j++)
            {
                if(str[j] == ' ')
                    channelNum++;
            }
        }
        ++col;
    }
    data_file.close();
    /*读取事件*/
    col = 0;
    std::map<std::string, long long> marks;
    event_file.open(eventsFile.toStdString(), std::ios::in);
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
    event_file.open(eventsFile.toStdString(), std::ios::in);
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
            eventLines.push_back(std::make_pair(event, (double)run_time / 10000.0));
        }
        ++col;
    }
    event_file.close();
    /*设置基本信息*/
    ui->label_5->setText(QString::number(channelNum));
    ui->label_7->setText(QString::number(col - 1));
    ui->label_9->setText("1");
    ui->label_11->setText(QString::number(SAMPLE_RATE) + "Hz");
    ui->label_13->setText(QString::number(allTime) + "s");
    /*内存分配*/
    channelsName = new std::string[channelNum];
    for(int i = 0; i < channelNum; i++)
        channelsName[i] = std::to_string(i + 1);
    /*初始化绘图板*/
    initChart(col - 1);
    hasOpen = true;
    /*画图*/
    paintChart();
}

/*从外部EDF/EDF+/BDF文件读取数据*/
void PreprocessWindow::readEDForBDF()
{
    if(!samplePoints.empty())
        samplePoints.clear();
    if(!eventLines.empty())
        eventLines.clear();
    /*用户选择文件路径*/
    QString filename;
    filename = QFileDialog::getOpenFileName(this,
        tr("选择EDF/EDF+/BDF文件"),
        "",
        tr("*.edf *.edf+ *.bdf")); //选择路径
    if(filename.isEmpty())
    {
        return;
    }
    /*读取EDF/EDF+头文件信息并显示*/
    edf_hdr_struct ehs;  // 头文件信息保存的结构体
    int flag = edfopen_file_readonly(filename.toStdString().c_str(), &ehs, EDFLIB_READ_ALL_ANNOTATIONS);
    double sampleFrequency = ehs.signalparam[0].smp_in_datarecord/(ehs.datarecord_duration/10000000);
    allTime = ehs.file_duration / (double)EDFLIB_TIME_DIMENSION;
    channelNum = ehs.edfsignals;
    ui->label_5->setText(QString::number(channelNum));
    ui->label_7->setText(QString::number(ehs.annotations_in_file));
    ui->label_9->setText("1");
    ui->label_11->setText(QString::number(sampleFrequency) + "Hz");
    ui->label_13->setText(QString::number(allTime) + "s");
    /*读取数据点*/
    int samplesNum = allTime * sampleFrequency;
    for(int i = 0; i < channelNum; i++)
    {
        double *buf = new double[samplesNum];
        edfread_physical_samples(flag, i, samplesNum, buf);
        for(int j = 0; j < samplesNum; j++)
        {
            samplePoints[i].push_back(QPointF((double)j / sampleFrequency, buf[j]));
        }
        delete []buf;
    }
    /*读取事件*/
    for(int i = 1; i < ehs.annotations_in_file; i++)
    {
        edf_annotation_struct eas;
        edf_get_annotation(flag, i, &eas);
        eventLines.push_back(std::make_pair(std::string(eas.annotation), eas.onset / (double)EDFLIB_TIME_DIMENSION));
    }
    /*关闭文件*/
    edfclose_file(flag);
    /*内存分配*/
    channelsName = new std::string[channelNum];
    for(int i = 0; i < channelNum; i++)
        channelsName[i] = std::to_string(i + 1);
    /*绘图板初始化*/
    initChart(ehs.annotations_in_file);
    hasOpen = true;
    /*画图*/
    paintChart();
}

/*绘图初始化*/
void PreprocessWindow::initChart(int markNum)
{
    /*初始化*/
    if(!hasOpen)
    {
        help = new ChartHelp(this);
        axisX = new QValueAxis;
        axisY = new QCategoryAxis;
        chart = new QChart;
    }
    else
    {
        std::size_t i;
        for(i = 0; i < lines.size(); i++)
            delete lines[i];
        for(i = 0; i < series.size(); i++)
        {
            delete series[i];
        }
        for(i = 0; i < pItems.size(); i++)
            delete pItems[i];
        std::vector<QSplineSeries *> m;
        series.swap(m);
    }
    for(int i = 0; i < channelNum; i++)
    {
        series.push_back(new QSplineSeries);
    }
    //设置x轴
    axisX->setRange(0, interval);
    axisX->setTickCount(5);
    chart->addAxis(axisX, Qt::AlignBottom);
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
    chart->axisX()->setGridLineVisible(false);
    chart->layout()->setContentsMargins(0, 0, 0, 0);//设置外边界
    chart->setMargins(QMargins(20, 50, 0, 0));//设置内边界
    chart->setBackgroundRoundness(0);//设置背景区域无圆角
    help->ui->widget_pre_wave->setChart(chart);
    QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
    item->setSizeHint(QSize(1300, 650));
    ui->listWidget->setItemWidget(item, help->ui->widget_pre_wave);
    /*初始化Mark名称数组*/
    for(int i = 0; i < markNum; i++)
        pItems.push_back(new QGraphicsSimpleTextItem(chart));
}

/*绘图*/
void PreprocessWindow::paintChart()
{
    /*清空之前的内容*/
    std::size_t i;
    for(int j = 0; j < channelNum; j++)
    {
        series[j]->clear();
        axisY->remove(QString::fromStdString(channelsName[j]));
    }
    /*设置起点与终点*/
    double s, e;
    if(isJmp)
    {
        s = jmp_start;
        e = jmp_end;
    }
    else
    {
        s = start;
        e = s + interval;
    }
    /*设置坐标轴范围*/
    axisX->setRange(s, e);
    axisY->setMax(channelNum * maxVotagle * 2);
    /*设置通道标签*/
    for(int j = channelNum - 1; j >= 0; j--)
        axisY->append(QString::fromStdString(channelsName[j]), maxVotagle * (2 * channelNum - 2 * j - 1));
    /*绘制数据点*/
    std::map<int, std::vector<QPointF>>::iterator sample_iter;
    for(sample_iter = samplePoints.begin(); sample_iter != samplePoints.end(); sample_iter++)
    {
        for(unsigned int i = 0; i < sample_iter->second.size(); i++)
        {
            if(sample_iter->second[i].x() >= s && sample_iter->second[i].x() < e)
                series[sample_iter->first]->append(QPointF(sample_iter->second[i].x(),
                                                           isOverlapping
                                                         ? sample_iter->second[i].y() + channelNum * maxVotagle
                                                         : sample_iter->second[i].y() + maxVotagle * (2 * channelNum - 2 * sample_iter->first - 1)));
        }
    }
    /*绘制事件*/
    for(i = 0; i < eventLines.size(); i++)
    {
        /*画直线*/
        QLineSeries *line = new QLineSeries;
        QPen splinePen;
        splinePen.setColor(markColors[eventLines[i].first]);
        line->setPen(splinePen);
        line->append(QPointF(eventLines[i].second, 0));
        line->append(QPointF(eventLines[i].second, channelNum * maxVotagle * 2));
        chart->addSeries(line);
        chart->setAxisX(axisX, line);
        chart->setAxisY(axisY, line);
        lines.push_back(line);
        /*显示文字标记*/
        pItems[i]->setText(QString::fromStdString(eventLines[i].first));
        pItems[i]->setPos(chart->mapToPosition(QPointF(eventLines[i].second, channelNum * maxVotagle * 2 -  maxVotagle), line));
    }
}

/*生成随机颜色用于不同的Mark*/
QColor PreprocessWindow::getRadomColor(int baseR, int baseG, int baseB)
{
    return QColor(baseR + (-10 + rand() % 20), baseG + (-10 + rand() % 20), baseB + (-10 + rand() % 20));
}

void PreprocessWindow::on_comboBox_currentIndexChanged(int index)
{
    if(index == 0)
        interval = 1;
    else if(index == 1)
        interval = 5;
    else
        interval = 10;
    paintChart();
}

void PreprocessWindow::on_comboBox_2_currentIndexChanged(int index)
{
    if(index == 0)
        maxVotagle = 50;
    else if(index == 1)
        maxVotagle = 100;
    else
        maxVotagle = 200;
    /*重新绘图*/
    paintChart();
}

void PreprocessWindow::on_pushButton_clicked()
{
    emit returnMain();
}

void PreprocessWindow::on_pushButton_2_clicked()
{
    if(start >= interval)
    {
        start -= interval;
        end -= interval;
        paintChart();
    }
}

void PreprocessWindow::on_pushButton_3_clicked()
{
    if(end + interval <= allTime)
    {
        start += interval;
        end += interval;
        paintChart();
    }
}

/*切换显示区域*/
void PreprocessWindow::on_pushButton_4_clicked()
{
    if(jmp_start >= 0.0 && jmp_end >= 0.0 && jmp_start < jmp_end && jmp_end < allTime)
    {
        paintChart();
        start = jmp_start;
        end = jmp_end;
    }
    isJmp = false;
}

/*波形重叠/取消重叠复用按钮*/
void PreprocessWindow::on_pushButton_5_clicked()
{
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

void PreprocessWindow::on_lineEdit_editingFinished()
{
    try
    {
        jmp_start = ui->lineEdit->text().toDouble();
    }
    catch (...)
    {
        return;
    }

}

void PreprocessWindow::on_lineEdit_2_editingFinished()
{
    try
    {
        jmp_end = ui->lineEdit_2->text().toDouble();
        isJmp = true;
    }
    catch (...)
    {
        isJmp = false;
        return;
    }
}

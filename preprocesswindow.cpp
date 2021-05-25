#include "preprocesswindow.h"
#include "ui_preprocesswindow.h"
#include "ui_charthelp.h"

PreprocessWindow::PreprocessWindow(QWidget *parent) :
    QMainWindow(parent),
    interval(1), channelNum(1), maxVotagle(50),
    startTimePSD(0.0), stopTimePSD(0.0), freqMin(0), freqMax(0), start(0.0), end(5.0), jmp_start(-1.0), jmp_end(-1.0), allTime(0.0),
    hasOpen(false), isJmp(false), isOverlapping(false),
    channelsName(nullptr),
    ui(new Ui::PreprocessWindow)
{
    ui->setupUi(this);
    /*设置python.exe所在路径*/
    Py_SetPythonHome(L"E:\\Anaconda3");
    //模块初始化
    Py_Initialize();
    if (!Py_IsInitialized())
    {
        QMessageBox::critical(this, this->tr("错误"), "模块加载错误!", QMessageBox::Ok);
        return;
    }
    pModule = PyImport_ImportModule("dataformatload");
    if (!pModule)
    {
        QMessageBox::critical(this, this->tr("错误"), "目标模块无法打开！");
    }
    /*链接信号与槽*/
    connect(this, SIGNAL(returnMain()), parent, SLOT(goToMainWindow()));
    connect(ui->actionlocalFile, SIGNAL(triggered()), this, SLOT(readDataFromLocal()));
    connect(ui->actionedf, SIGNAL(triggered()), this, SLOT(readEDF()));
    connect(ui->actioneeg_2, SIGNAL(triggered()), this, SLOT(readEEG()));
    connect(ui->actionrenameMotage, SIGNAL(triggered()), this, SLOT(setChannelsName()));
    connect(ui->actionFIR, SIGNAL(triggered()), this, SLOT(filt()));
    connect(ui->actionPSD, SIGNAL(triggered()), this, SLOT(plotPSD()));
    connect(ui->actionWigner, SIGNAL(triggered()), this, SLOT(plotWigner()));
    connect(ui->actionDWT, SIGNAL(triggered()), this, SLOT(plotDWT()));
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
    delete p;
    delete psi;
    delete wi;
    delete w;
    delete ui;
}

/*==================================== 波形显示 ======================================*/

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
    samplesFile = tempFile + "_samples.txt";
    eventsFile = tempFile + "_events.txt";
    std::cout << samplesFile.toStdString() << std::endl;
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
    list = PyList_New(0);
    data_file.open(samplesFile.toStdString(), std::ios::in);
    /*计算总时间*/
    while(data_file.peek() != EOF)
    {
        std::string str;
        std::getline(data_file, str);
        ++col;
    }
    allTime = (double)col  / (double)sampleFreq;
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
                samplePoints[j].push_back(QPointF((double)col / (double)sampleFreq, m));
                PyList_Append(list, Py_BuildValue("d", m));
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
    ui->label_11->setText(QString::number(sampleFreq) + "Hz");
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
    /*导出raw对象*/
    arg = PyTuple_New(3);
    PyTuple_SetItem(arg, 0, Py_BuildValue("i", channelNum));
    PyTuple_SetItem(arg, 1, Py_BuildValue("d", sampleFreq));
    PyTuple_SetItem(arg, 2, list);
    pFun = PyObject_GetAttrString(pModule, "makeRaw");
    if(!pFun)
    {
        QMessageBox::critical(this, this->tr("错误"), "未找到目标函数!", QMessageBox::Ok);
        return;
    }
    PyObject_CallObject(pFun, arg);
}

/*从外部EDF/EDF+文件读取数据*/
void PreprocessWindow::readEDF()
{
    if(!samplePoints.empty())
        samplePoints.clear();
    if(!eventLines.empty())
        eventLines.clear();
    /*用户选择文件路径*/
    filePath = QFileDialog::getOpenFileName(this,
        tr("选择edf文件"),
        "",
        tr("*.edf *.EDF"));
    if(filePath.isEmpty())
    {
        return;
    }
    PyObject *sf, *cnList, *eventList, *eventDict, *key, *value, *dataList, *timeList;
   pFun= PyObject_GetAttrString(pModule,"readRawEDF");
   if(!pFun)
   {
       QMessageBox::critical(this, this->tr("错误"), "未找到目标函数!", QMessageBox::Ok);
       return;
   }
   /*是否从通道导入事件*/
   int eventIndex = -1;
   std::string eventChannel = "";
   int reply = QMessageBox::question(this, this->tr(""), "是否从通道内导入事件？", QMessageBox::Ok, QMessageBox::No);
   if(reply == QMessageBox::Ok)
   {
       SetEventChannel sec;
       if(sec.exec() == QDialog::Accepted)
       {
           eventChannel = sec.eventChannel.toStdString();
           arg = Py_BuildValue("(ss)", filePath.toStdString().c_str(), eventChannel.c_str());
       }
       else
       {
           return;
       }
   }
   else
   {
       arg = Py_BuildValue("(s)", filePath.toStdString().c_str());
   }
   PyObject_CallObject(pFun, arg);
   pFun= PyObject_GetAttrString(pModule,"getErrorFlag");
   error = PyObject_CallFunction(pFun, nullptr);
   if(PyLong_AsLong(error) != 0)
   {
       QMessageBox::critical(this, this->tr("错误"), "该事件通道不存在！", QMessageBox::Ok);
       return;
   }
   int i, j, cnLen, eventListLen, dataListLen0, dataListLen1;
   // 得到采样率
   pFun= PyObject_GetAttrString(pModule,"getSampleFreq");
   sf = PyObject_CallFunction(pFun, nullptr);
   sampleFreq = PyFloat_AsDouble(sf);
   // 得到通道名称
   pFun= PyObject_GetAttrString(pModule,"getChannelName");
   cnList = PyObject_CallFunction(pFun, nullptr);
   cnLen = PyObject_Size(cnList);
   channelsName = new std::string[cnLen];
   for(i = 0; i < cnLen; i++)
   {
       if(eventChannel != "")
       {
           if(PyUnicode_AsUTF8(PyList_GetItem(cnList, i)) != eventChannel)
               channelsName[i] = PyUnicode_AsUTF8(PyList_GetItem(cnList, i));
           else
               eventIndex = i;
       }
       else
            channelsName[i] = PyUnicode_AsUTF8(PyList_GetItem(cnList, i));
   }
   // 得到事件id的映射
   pFun= PyObject_GetAttrString(pModule,"getEventDict");
   eventDict = PyObject_CallFunction(pFun, nullptr);
   std::map<int, string> markDict;
   Py_ssize_t pos = 0;
   while(PyDict_Next(eventDict, &pos, &key, &value))
   {
        markDict[PyLong_AsLong(value)] = PyUnicode_AsUTF8(key);
   }
   // 得到事件
   pFun= PyObject_GetAttrString(pModule,"getEvents");
   eventList = PyObject_CallFunction(pFun, nullptr);
   eventListLen = PyObject_Size(eventList);
   for(i = 0; i < eventListLen; i++)
   {
       PyObject *row = PyList_GetItem(eventList, i);
       PyObject *time = PyList_GetItem(row, 0);
       PyObject *eventId = PyList_GetItem(row, 2);
       if(markDict.empty())
            eventLines.push_back(std::make_pair(std::to_string(PyLong_AsLong(eventId)), PyLong_AsLongLong(time) / sampleFreq));
       else
            eventLines.push_back(std::make_pair(markDict[PyLong_AsLong(eventId)], PyLong_AsLongLong(time) / sampleFreq));
   }
   // 得到时间
   pFun = PyObject_GetAttrString(pModule,"getTime");
   timeList = PyObject_CallFunction(pFun, nullptr);
   // 得到数据点
   pFun= PyObject_GetAttrString(pModule,"getData");
   dataList = PyObject_CallFunction(pFun, nullptr);
   dataListLen0 = PyObject_Size(dataList);
   for(i = 0; i < dataListLen0; i++)
   {
       if((eventIndex != -1) && (i == eventIndex))
           continue;
       PyObject *row = PyList_GetItem(dataList, i);
       dataListLen1 = PyObject_Size(row);
       for(j = 0; j < dataListLen1; j++)
       {
           samplePoints[i].push_back(QPointF(PyFloat_AsDouble(PyList_GetItem(timeList, j)), PyFloat_AsDouble(PyList_GetItem(row, j))));
       }
   }
   /*设置标志量*/
   channelNum = ((eventIndex == -1) ? cnLen : cnLen - 1);
   allTime = PyFloat_AsDouble(PyList_GetItem(timeList, dataListLen1 - 1));
   /*设置基本信息*/
   ui->label_5->setText(QString::number(channelNum));
   ui->label_7->setText(QString::number(eventListLen));
   ui->label_9->setText("1");
   ui->label_11->setText(QString::number(sampleFreq) + "Hz");
   ui->label_13->setText(QString::number(allTime) + "s");
   /*绘图板初始化*/
   initChart(eventListLen);
   hasOpen = true;
   /*画图*/
   paintChart();
}

/*读取eeg文件*/
void PreprocessWindow::readEEG()
{
    if(!samplePoints.empty())
        samplePoints.clear();
    if(!eventLines.empty())
        eventLines.clear();
    /*用户选择文件路径*/
    QString filename;
    filename = QFileDialog::getOpenFileName(this,
        tr("选择vhdr文件"),
        "",
        tr("*.vhdr *.VHDR")); //选择路径
    if(filename.isEmpty())
    {
        return;
    }
    Py_Initialize();
    if (!Py_IsInitialized())
    {
        QMessageBox::critical(this, this->tr("错误"), "模块初始化错误!", QMessageBox::Ok);
        return;
    }
    PyObject *pModule, *pFun, *arg, *sf, *cnList, *eventList, *eventDict, *key, *value, *dataList, *timeList;
    pModule = PyImport_ImportModule("dataformatload");
    if (!pModule)
    {
        QMessageBox::critical(this, this->tr("错误"), "打不开Python目标");
    }
   pFun= PyObject_GetAttrString(pModule,"readRawEEG");
   if(!pFun)
   {
       QMessageBox::critical(this, this->tr("错误"), "未找到目标函数!", QMessageBox::Ok);
       return;
   }
   arg = Py_BuildValue("(s)", filename.toStdString().c_str());
   PyObject_CallObject(pFun, arg);
   int i, j, cnLen, eventListLen, dataListLen0, dataListLen1;
   // 得到采样率
   pFun= PyObject_GetAttrString(pModule,"getSampleFreq");
   sf = PyObject_CallFunction(pFun, nullptr);
   sampleFreq = PyFloat_AsDouble(sf);
   // 得到通道名称
   pFun= PyObject_GetAttrString(pModule,"getChannelName");
   cnList = PyObject_CallFunction(pFun, nullptr);
   cnLen = PyObject_Size(cnList);
   channelsName = new std::string[cnLen];
   for(i = 0; i < cnLen; i++)
   {
       channelsName[i] = PyUnicode_AsUTF8(PyList_GetItem(cnList, i));
   }
   // 得到事件id的映射
   pFun= PyObject_GetAttrString(pModule,"getEventDict");
   eventDict = PyObject_CallFunction(pFun, nullptr);
   std::map<int, string> markDict;
   Py_ssize_t pos = 0;
   while(PyDict_Next(eventDict, &pos, &key, &value))
   {
        markDict[PyLong_AsLong(value)] = PyUnicode_AsUTF8(key);
   }
   // 得到事件
   pFun= PyObject_GetAttrString(pModule,"getEvents");
   eventList = PyObject_CallFunction(pFun, nullptr);
   eventListLen = PyObject_Size(eventList);
   for(i = 0; i < eventListLen; i++)
   {
       PyObject *row = PyList_GetItem(eventList, i);
       PyObject *time = PyList_GetItem(row, 0);
       PyObject *eventId = PyList_GetItem(row, 2);
       eventLines.push_back(std::make_pair(markDict[PyLong_AsLong(eventId)], PyLong_AsLongLong(time) / sampleFreq));
   }
   // 得到时间
   pFun = PyObject_GetAttrString(pModule,"getTime");
   timeList = PyObject_CallFunction(pFun, nullptr);
   // 得到数据点
   pFun= PyObject_GetAttrString(pModule,"getData");
   dataList = PyObject_CallFunction(pFun, nullptr);
   dataListLen0 = PyObject_Size(dataList);
   for(i = 0; i < dataListLen0; i++)
   {
       PyObject *row = PyList_GetItem(dataList, i);
       dataListLen1 = PyObject_Size(row);
       for(j = 0; j < dataListLen1; j++)
       {
           samplePoints[i].push_back(QPointF(PyFloat_AsDouble(PyList_GetItem(timeList, j)), PyFloat_AsDouble(PyList_GetItem(row, j))));
       }
   }
   Py_Finalize();
   /*设置标志量*/
   channelNum = cnLen;
   allTime = PyFloat_AsDouble(PyList_GetItem(timeList, dataListLen1 - 1));
   /*设置基本信息*/
   ui->label_5->setText(QString::number(channelNum));
   ui->label_7->setText(QString::number(dataListLen1));
   ui->label_9->setText("1");
   ui->label_11->setText(QString::number(sampleFreq) + "Hz");
   ui->label_13->setText(QString::number(allTime) + "s");
   /*绘图板初始化*/
   initChart(eventListLen);
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
    if(channelNum > 16)
        axisY->setLabelsFont(QFont("Times", 6));
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
    item->setSizeHint(QSize(1400, 800));
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
    /*绘制数据点(1s时间间隔绘制100个点)*/
    std::map<int, std::vector<QPointF>>::iterator sample_iter;
    for(sample_iter = samplePoints.begin(); sample_iter != samplePoints.end(); sample_iter++)
    {
        for(unsigned int i = 0; i < sample_iter->second.size(); i++)
        {
            if(sample_iter->second[i].x() >= s && sample_iter->second[i].x() < e)
            {
                series[sample_iter->first]->append(QPointF(sample_iter->second[i].x(),
                                                   isOverlapping
                                                 ? sample_iter->second[i].y() + channelNum * maxVotagle
                                                 : sample_iter->second[i].y() + maxVotagle * (2 * channelNum - 2 * sample_iter->first - 1)));
            }
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

/*==================================== 滤波 ======================================*/
/*得到滤波器设置*/
void PreprocessWindow::receiveFilterInfo(double l, double h, int o)
{
    this->lowPass = l;
    this->highPass = h;
    this->order = o;
}

/*滤波并显示波形*/
void PreprocessWindow::filt()
{
    filterSetting *fs = new filterSetting(this);
    connect(fs, SIGNAL(sendFilterSetting(double, double, int)), this, SLOT(receiveFilterInfo(double, double, int)));
    if(fs->exec() == QDialog::Accepted)
    {
        if((lowPass > 0.0) && (highPass > 0.0))
        {
            int k;
            MyFilter f;
            double y_n, coff[order];
            QQueue<double> buf[channelNum];
            f.countBandPassCoef(order, sampleFreq, coff, lowPass, highPass);  // 计算滤波器系数
            // 重新计算数据点y值
            std::map<int, std::vector<QPointF>>::iterator sample_iter;
            for(sample_iter = samplePoints.begin(); sample_iter != samplePoints.end(); sample_iter++)
            {
                for(size_t i = 0; i < sample_iter->second.size(); i++)
                {
                    if(buf[sample_iter->first].size() <= order)
                        buf[sample_iter->first].enqueue((sample_iter->second)[i].y());
                    else
                    {
                        /*计算滤波后的值*/
                        y_n = 0.0;
                        for(k = 0; k <= order; k++)
                        {
                            y_n += coff[k] * buf[sample_iter->first][order - k];
                        }
                        /*队列左移一位*/
                        buf[sample_iter->first].dequeue();
                        /*原始值入队*/
                        buf[sample_iter->first].enqueue((sample_iter->second)[i].y());
                        /*替换数据点*/
                        (sample_iter->second)[i] = QPointF((sample_iter->second)[i].x(), y_n);
                    }
                }
            }
            // 重新绘图
            paintChart();
            // raw对象滤波
            arg = Py_BuildValue("(dd)", lowPass, highPass);
            pFun = PyObject_GetAttrString(pModule,"filt");
            if(!pFun)
            {
                QMessageBox::critical(this, this->tr("错误"), "未找到目标函数!", QMessageBox::Ok);
                return;
            }
            PyObject_CallObject(pFun, arg);
        }
        else
            QMessageBox::critical(this, this->tr("错误"), "所需信息未填写或数值错误！", QMessageBox::Ok);
    }
}

/*==================================== 功率谱估计 ======================================*/
void PreprocessWindow::getStartTimePSD(double a)
{
    this->startTimePSD = a;
}

void PreprocessWindow::getStopTimePSD(double a)
{
    this->stopTimePSD = a;
}

void PreprocessWindow::plotPSD()
{
    psi = new PSDInfo(this);
    p = new PSD(channelNum, sampleFreq);
    connect(psi, SIGNAL(sendStartTime(double)), this, SLOT(getStartTimePSD(double)));
    connect(psi, SIGNAL(sendStopTime(double)), this, SLOT(getStopTimePSD(double)));
    connect(psi, SIGNAL(sendStartFreq(double)), p, SLOT(getStartFreq(double)));
    connect(psi, SIGNAL(sendStopFreq(double)), p, SLOT(getStopFreq(double)));
    connect(psi, SIGNAL(sendPSDType(PSDType)), p, SLOT(getPSDType(PSDType)));
    if(psi->exec() == QDialog::Accepted)
    {
        if((stopTimePSD == 0.0) || (stopTimePSD > allTime))
            stopTimePSD = allTime;
        if(startTimePSD >= stopTimePSD)
        {
            QMessageBox::critical(this, this->tr("错误"), "参数填写错误！");
            return;
        }
        int k, len = samplePoints[0].size();
        unsigned int i;
        double *points[channelNum];
        for(k = 0; k < channelNum; k++)
            points[k] = new double[len];
        std::map<int, std::vector<QPointF> >::iterator sample_iter;
        for(sample_iter = samplePoints.begin(); sample_iter != samplePoints.end(); sample_iter++)
        {
            k = 0;
            for(i = 0; i < sample_iter->second.size(); i++)
            {
                if(sample_iter->second[i].x() >= startTimePSD)
                {
                     points[sample_iter->first][k++] = sample_iter->second[i].y();
                }
            }
        }
        p->plot(points, len);
        p->show();
    }
}

/*==================================== 维格纳分布 ======================================*/
void PreprocessWindow::getBeginTime(double a)
{
    this->startTimeWigner = a;
}

void PreprocessWindow::getEndTime(double a)
{
    this->endTimeWigner = a;
}

void PreprocessWindow::getChannel(QString a)
{
    this->channel = a;
}

void PreprocessWindow::plotWigner()
{
    wi = new WignerInfo(this);
    w = new Wigner(channelNum, sampleFreq);
    connect(wi, SIGNAL(sendBeginTime(double)), this, SLOT(getBeginTime(double)));
    connect(wi, SIGNAL(sendEndTime(double)), this, SLOT(getEndTime(double)));
    connect(wi, SIGNAL(sendBeginTime(double)), w, SLOT(getBeginTime(double)));
    connect(wi, SIGNAL(sendEndTime(double)), w, SLOT(getEndTime(double)));
    connect(wi, SIGNAL(sendChannelName(QString)), w, SLOT(getChannel(QString)));
    connect(wi, SIGNAL(sendChannelName(QString)), this, SLOT(getChannel(QString)));
    if(wi->exec() == QDialog::Accepted)
    {
        // 判断参数是否合法
        int i;
        bool isFind = false;
        for(i = 0; i < channelNum; i++)
        {
            if(channel.toStdString() == channelsName[i])
            {
                isFind = true;
                break;
            }
        }
        if(isFind)
        {
            int j, k = 0, len = samplePoints[i].size();
            std::vector<double> points;
            for(j = 0; j < len; j++)
            {
                if(((int)samplePoints[i][j].x() == k))
                {
                    points.push_back(samplePoints[i][j].y());
                    ++k;
                }
            }
            w->plotWigner(points);
            w->show();
        }
        else
            QMessageBox::critical(this, this->tr("错误"), "参数填写错误！");
    }
}

void PreprocessWindow::getFreqMin(double a)
{
    this->freqMin = a;
    if(!this->freqMin)
        ++(this->freqMin);
}

void PreprocessWindow::getFreqMax(double a)
{
    this->freqMax = a;
}

void PreprocessWindow::plotDWT()
{
    di = new DwtInfo(this);
    connect(di, SIGNAL(sendFreqMin(double)), this, SLOT(getFreqMin(double)));
    connect(di, SIGNAL(sendFreqMax(double)), this, SLOT(getFreqMax(double)));
    connect(di, SIGNAL(sendChannel(QString)), this, SLOT(getChannel(QString)));
    //绘图
    if(di->exec() == QDialog::Accepted)
    {
        // 判断参数是否合法
        int i;
        bool isFind = false;
        for(i = 0; i < channelNum; i++)
        {
            if(channel.toStdString() == channelsName[i])
            {
                isFind = true;
                break;
            }
        }
        if(isFind && (freqMin > 0) && (freqMax > freqMin))
        {
            arg = Py_BuildValue("(iis)", freqMin, freqMax, channel.toStdString().c_str());
            pFun = PyObject_GetAttrString(pModule,"plotDWT");
            if(!pFun)
            {
                QMessageBox::critical(this, this->tr("错误"), "未找到目标函数!", QMessageBox::Ok);
                return;
            }
            PyObject_CallObject(pFun, arg);
            pFun= PyObject_GetAttrString(pModule,"getErrorFlag");
            error = PyObject_CallFunction(pFun, nullptr);
            if(PyLong_AsLong(error) != 0)
            {
                QMessageBox::critical(this, this->tr("错误"), "输入信号长度至少应大于小波长度", QMessageBox::Ok);
                return;
            }
        }
        else
        {
            QMessageBox::critical(this, this->tr("错误"), "参数填写错误！");
            return;
        }
    }
}

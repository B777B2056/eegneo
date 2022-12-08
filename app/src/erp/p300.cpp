#include "erp/p300.h"
#include "ui_p300.h"

#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

P300Oddball::P300Oddball(QWidget *parent)
    : QWidget(parent)
    , isTrain(true), isExp(false),isFinish(false) , imgCnt(0), cnt_2(0)
    , ui(new Ui::p300)
{
    ui->setupUi(this);
    QPalette palette(this->palette());
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);
    // 显示实验参数设置对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("实验参数设置"),
                                    "默认参数（单Trial）如下\n"
                                    "刺激出现概率(数字2)：20%\n"
                                    "刺激出现前提示时间：800ms\n"
                                    "刺激持续时间：50ms\n"
                                    "数字闪烁间隔：1000ms~1200ms\n"
                                    "是否自定义实验参数?",
                                    QMessageBox::Ok | QMessageBox::No);
    if (reply == QMessageBox::Ok){
        p300oddballsetting scl;
  Again:
        int rec = scl.exec();
        if(rec == QDialog::Accepted)
        {
            num_img = scl.num_img;
            freq_2 = scl.freq_2;
            crossDurationTime = scl.crossDurationTime;
            numDurationTime = scl.numDurationTime;
            blankDurationTimeDown = scl.blankDurationTimeDown;
            blankDurationTimeUp = scl.blankDurationTimeUp;
            if(freq_2 >= 1)
            {
                int reply = QMessageBox::critical(this, tr("错误"), "频率不可大于等于1！", QMessageBox::Ok);
                if(reply == QMessageBox::Ok)
                    goto Again;
            }
            if(blankDurationTimeDown > blankDurationTimeUp)
            {
                int reply = QMessageBox::critical(this, tr("错误"), "时间下限不可超过时间上限！", QMessageBox::Ok);
                if(reply == QMessageBox::Ok)
                    goto Again;
            }
            if(freq_2 <= 0 || num_img <= 0 || crossDurationTime <= 0 || numDurationTime <= 0 || blankDurationTimeDown < 0)
            {
                int reply = QMessageBox::critical(this, tr("错误"), "参数不可为负数！", QMessageBox::Ok);
                if(reply == QMessageBox::Ok)
                    goto Again;
            }
        }
        else
        {
            goto Again;
        }
    }
    else{
        /*设置默认参数*/
        num_img = 200;
        freq_2 = 0.2;
        crossDurationTime = 800;
        numDurationTime = 50;
        blankDurationTimeDown = 1000;
        blankDurationTimeUp = 1200;
    }
    // 发送图片总数量至采集界面
    emit sendImgNum(num_img);
    // 显示准备界面
    ui->label->setAlignment(Qt::AlignCenter);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/gotask.jpg"));
    ui->label->show();
    // 全屏显示
//    this->showFullScreen();
}

P300Oddball::~P300Oddball()
{
    delete ui;
}

void P300Oddball::keyPressEvent(QKeyEvent *event)
{
    // 空按格或数字1开始练习
    if(isTrain && (event->key() == Qt::Key_Space || event->key() == Qt::Key_1))
    {
        while(imgCnt < 40)
        {
            playImages(40);
            ++imgCnt;
        }
        stopTrain();
    }
    // 按2开始正式实验
    if(!isTrain && event->key() == Qt::Key_2)
    {
        isExp = true;
        emit sendMark("Start");
        while(imgCnt < num_img)
        {
            playImages(num_img);
            ++imgCnt;
        }
        stopExperiment();
    }
    // 结束后可按Esc退出全屏
    if(isFinish && event->key() == Qt::Key_Escape)
        this->showNormal();
}

void P300Oddball::mousePressEvent(QMouseEvent *event)
{
    if(isExp && event->button() == Qt::LeftButton)
    {
        emit sendMark("Response");
    }
}

void P300Oddball::playImages(int imgNum)
{
    // QTime cross_duration, num_duration, blank_duration;
    // // 十字准星显示周期（默认）：800ms
    // cross_duration.start();
    // ui->label->setVisible(true);
    // ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/cross.png"));
    // while(cross_duration.elapsed() < crossDurationTime)
    // {
    //     QCoreApplication::processEvents();
    // }
    // // 数字显示周期（默认）：50ms
    // num_duration.start();
    // // 产生随机数
    // int n = std::rand() % imgNum;
    // if(n < (int)(imgNum * freq_2) && cnt_2 < (int)(imgNum * freq_2))
    // {
    //     ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/2.bmp"));
    //     if(!isTrain)
    //         emit sendMark("2");
    //     cnt_2++;
    // }
    // else
    // {
    //     ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/8.bmp"));
    //     if(!isTrain)
    //         emit sendMark("8");
    // }
    // while(num_duration.elapsed() < numDurationTime)
    // {
    //     QCoreApplication::processEvents();
    // }
    // // 空白周期（默认）：1000ms-1200ms
    // ui->label->setVisible(false);
    // blank_duration.start();
    // while(blank_duration.elapsed() < (blankDurationTimeDown + rand() % (blankDurationTimeUp - blankDurationTimeDown)))
    // {
    //     QCoreApplication::processEvents();
    // }
}

void P300Oddball::stopTrain()
{
    isTrain = false;
    // 显示选择界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/ifgo.jpg"));
    // 计数器重置
    cnt_2 = 0;
    imgCnt = 0;
}

void P300Oddball::stopExperiment()
{
    // 显示结束界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/end.jpg"));
    emit sendMark("End");
    // 可退出全屏标志位
    isFinish = true;
}

void P300Oddball::closeEvent(QCloseEvent *event)
{
    this->blockSignals(true);
    event->accept();
}

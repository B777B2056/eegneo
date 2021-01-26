#include "p300.h"
#include "ui_p300.h"

P300Oddball::P300Oddball(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::p300)
{
    ui->setupUi(this);
    QPalette palette(this->palette());
    palette.setColor(QPalette::Background, Qt::black);
    this->setPalette(palette);
    //显示准备界面
    ui->label->setAlignment(Qt::AlignCenter);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/gotask.jpg"));
    ui->label->show();
}

P300Oddball::~P300Oddball()
{
    delete train_timer;
    delete train_duration;
    delete experiment_timer;
    delete experiment_duration;
    delete ui;
}

void P300Oddball::keyPressEvent(QKeyEvent *event)
{
    //空按格或数字1开始练习
    if(isTrain && (event->key() == Qt::Key_Space || event->key() == Qt::Key_1))
    {
        train_timer = new QTimer(this);
        train_timer->setInterval(500);
        connect(train_timer, SIGNAL(timeout()), SLOT(playNumsTrain()));
        train_duration = new QTimer(this);
        train_duration->setSingleShot(true);
        train_duration->setInterval(80000);
        connect(train_duration, SIGNAL(timeout()), SLOT(stopTrain()));
        train_timer->start();
        train_duration->start();
    }
    //按2开始正式实验
    if(!isTrain && event->key() == Qt::Key_2)
    {
        experiment_timer = new QTimer(this);
        experiment_timer->setInterval(500);
        connect(experiment_timer, SIGNAL(timeout()), SLOT(playNumsExperiment()));
        experiment_duration = new QTimer(this);
        experiment_duration->setSingleShot(true);
        experiment_duration->setInterval(400000);  // 400s
        connect(experiment_duration, SIGNAL(timeout()), SLOT(stopExperiment()));
        experiment_timer->start();
        experiment_duration->start();
        emit sendMark("Start");
        this->isExp = true;
    }
}

void P300Oddball::mousePressEvent(QMouseEvent *event)
{
    if(isExp && event->button() == Qt::LeftButton)
    {
        emit sendMark("Response");
    }
}

void P300Oddball::stopTrain()
{
    train_timer->stop();
    train_duration->stop();
    isTrain = false;
    //显示选择界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/ifgo.jpg"));
    //计数器重置
    cnt_2 = 0;
}

void P300Oddball::playNumsTrain()
{
    if(train_cnt > 3)
        train_cnt = 0;
    if(train_cnt == 0)
    {
        ui->label->setVisible(true);
        // 产生0~30的随机数
        int n = std::rand() % 40;
        if(n < 8 && cnt_2 < 8)
        {
            ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/2.bmp"));
            cnt_2++;
        }
        else
        {
            ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/8.bmp"));
        }     
    }
    else
    {
        ui->label->setVisible(false);
    }
    train_cnt++;
}

void P300Oddball::playNumsExperiment()
{
    if(experiment_cnt > 3)
        experiment_cnt = 0;
    if(experiment_cnt == 0)
    {
        ui->label->setVisible(true);
        // 产生0~200的随机数
        int n = std::rand() % 200;
        // 20%概率产生2
        if(n < 40 && cnt_2 < 40)
        {
            ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/2.bmp"));
            emit sendMark("2");
            cnt_2++;
        }
        // 80%概率产生8
        else
        {
            ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/8.bmp"));
            emit sendMark("8");
        }
    }
    else
    {
        ui->label->setVisible(false);
    }
    experiment_cnt++;
}

void P300Oddball::stopExperiment()
{
    experiment_timer->stop();
    experiment_duration->stop();
    //显示结束界面
    ui->label->setVisible(true);
    ui->label->setPixmap(QPixmap("../EEG_Acquisition_GUI/p300/oddball/end.jpg"));
    emit sendMark("End");
}

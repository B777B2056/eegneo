#ifndef P300_H
#define P300_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <fstream>

namespace Ui {
class p300;
}

/*P300 Oddball范式*/
class P300Oddball : public QWidget
{
    Q_OBJECT

protected:
    void keyPressEvent(QKeyEvent *event);   // 键盘按下事件
    void mousePressEvent(QMouseEvent *event);  // 鼠标响应事件

public:
    explicit P300Oddball(QWidget *parent = nullptr);
    ~P300Oddball();

signals:
    void sendMark(const std::string event);

private slots:
    void stopTrain();
    void stopExperiment();
    void playNumsTrain();  // 练习期间播放数字
    void playNumsExperiment();  // 正式实验期间播放数字、发送marker


private:
    bool isTrain = true, isExp = false;
    int train_cnt = 0, experiment_cnt = 0, cnt_2 = 0;
    QTimer *train_timer, *experiment_timer;  // 播放图片所用的定时器
    QTimer *train_duration, *experiment_duration;  // 训练过程与实验过程计时器(34s与200s)

    Ui::p300 *ui;
};

#endif // P300_H

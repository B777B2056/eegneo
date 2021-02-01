#ifndef P300_H
#define P300_H

#include <QWidget>
#include <QTime>
#include <QKeyEvent>
#include <QMouseEvent>
#include <fstream>
#include "p300oddballsetting.h"

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
    void closeEvent(QCloseEvent *event);  // 窗口关闭事件(实验进行期间关闭窗口将阻断mark继续发送)

public:
    explicit P300Oddball(QWidget *parent = nullptr);
    ~P300Oddball();

signals:
    void sendImgNum(int);
    void sendMark(const std::string event);

private:
    double freq_2;
    int num_img;
    int crossDurationTime, numDurationTime, blankDurationTimeDown, blankDurationTimeUp;
    bool isTrain, isExp, isFinish;
    int imgCnt, cnt_2;
    void playImages(int imgNum);  // 播放图片
    void stopTrain();
    void stopExperiment();

    Ui::p300 *ui;
};

#endif // P300_H

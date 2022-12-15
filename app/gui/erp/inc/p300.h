#pragma once
#include <QMainWindow>
#include <QKeyEvent>
#include <QMouseEvent>

namespace Ui {
class p300;
}

// P300 Oddball范式
class ErpP300OddballWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void keyPressEvent(QKeyEvent *event);   
    void mousePressEvent(QMouseEvent *event); 

public:
    explicit ErpP300OddballWindow(QWidget *parent = nullptr);
    ~ErpP300OddballWindow();

private:
    enum IMG_LABEL
    {
        TWO = 2,
        EIGHT = 8 
    };

private:
    double mStimulusImageRatio_;
    int mImagesInPracticeTotalCount_, mImagesInExperimentTotalCount_;
    int mCrossDurationMs_, mImageDurationMs_, mBlankDurationMsLowerBound_, mBlankDurationMsUpperBound_;
    bool mIsInPractice_, mIsInExperiment_, mIsEnded_;
    int mStimulusImageCount_;
    Ui::p300 *ui;

    void init();    // 从配置文件种读取参数
    IMG_LABEL chooseImg(int imgNumRound) const;
    void sendMarker(const char* msg);
    void playImagesRound(int imgNumRound);  // 播放图片
    void practiceEnd();
    void experimentEnd();
};

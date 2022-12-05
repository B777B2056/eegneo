#pragma once

#include <QMainWindow>
#include <memory>
#include "utils/mainbackground.h"
#include "acquisition/acquisitionwindow.h"
#include "analysis/preprocesswindow.h"
#include "ui_mainwindow.h"

class BackgroundWindow;
class AcquisitionWindow;
class PreprocessWindow;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void getBasicInfo(QString, QString);  // 获取基本信息
    void goToMainWindow();  // 返回主界面

private slots:
    void onPushButtonClicked();
    void onPushButton2Clicked();

private:
    QString participantNum, tempFiles;
    BackgroundWindow backgroundWindow;
    AcquisitionWindow acquisitionWindow;
    PreprocessWindow preprocessWindow;
    Ui::MainWindow *ui;

private:
    template<class Window>
    void goToOtherWindow(Window* target, bool isButtonVisable)
    {
        ui->stackedWidget->setCurrentWidget(target);
        ui->pushButton->setVisible(isButtonVisable);
        ui->pushButton->setEnabled(isButtonVisable);
        ui->pushButton_2->setVisible(isButtonVisable);
        ui->pushButton_2->setEnabled(isButtonVisable);
    }
};

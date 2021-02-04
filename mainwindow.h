#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <cstdlib>
#include "mainbackground.h"
#include "acquisitionwindow.h"
#include "preprocesswindow.h"

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
    void goToMainWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    QString participantNum, tempFiles;
    Background *b;
    AcquisitionWindow *m;
    PreprocessWindow *p;

    Ui::MainWindow *ui;
};

#endif // INITWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    explicit MainWindow(QString participantNum, QString date, QString others, QString expName, int cn, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void goToMainWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Background *b;
    AcquisitionWindow *m;
    PreprocessWindow *p;

    Ui::MainWindow *ui;
};

#endif // INITWINDOW_H

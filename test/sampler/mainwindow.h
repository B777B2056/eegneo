#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <array>
#include <cstddef>
#include <QMainWindow>
#include <QList>
#include <QSharedMemory>
#include "../../app/inc/acquisition/plotter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QChart;
class QValueAxis;
class QLineSeries;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void covert2Accquisition();
    void covert2Analysis();

private slots:
    void update();

private:
    Ui::MainWindow *ui;
    eegneo::EEGWavePlotter mChart_;
    QTimer* mTimer_;
    std::array<double, 8> mBuf_;
    QSharedMemory mSharedMemory_;
};
#endif // MAINWINDOW_H

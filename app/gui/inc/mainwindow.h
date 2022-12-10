#pragma once
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void covert2Accquisition();
    void covert2Analysis();

public slots:
    void covert2InitWindowImpl();  // 返回主界面

private slots:
    void covert2AccquisitionImpl();
    void covert2AnalysisImpl();

private:
    Ui::MainWindow *ui;
};

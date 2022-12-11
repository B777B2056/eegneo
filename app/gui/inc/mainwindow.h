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

    void covert2AccquisitionImpl();
    void covert2AnalysisImpl();

signals:
    void covert2Accquisition();
    void covert2Analysis();

private:
    Ui::MainWindow *ui;
};

#pragma once
#include <QMainWindow>
#include <memory>

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
    void basicInfo(QString, QString);  // 获取基本信息
    // void covert2InitWindow();  // 返回主界面

private slots:
    void covert2AccquisitionImpl();
    void covert2AnalysisImpl();

private:
    QString participantNum, tempFiles;
    Ui::MainWindow *ui;
};

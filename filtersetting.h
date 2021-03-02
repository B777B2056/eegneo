#ifndef FILTERSETTING_H
#define FILTERSETTING_H

#include <QDialog>
#include <QMessageBox>
#include <QCloseEvent>

namespace Ui {
class filterSetting;
}

class filterSetting : public QDialog
{
    Q_OBJECT

public:
    explicit filterSetting(QWidget *parent = nullptr);
    ~filterSetting();

signals:
    // 向预处理界面发送滤波器设置(低通频率，高通频率，阶数)
    void sendFilterSetting(double, double, int);

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_3_editingFinished();

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    double lowPass, highPass, order;
    Ui::filterSetting *ui;
};

#endif // FILTERSETTING_H

#pragma once
#include <QDialog>
#include "utils/enum.h"

namespace Ui {
class PSDInfo;
}

class PSDInfo : public QDialog
{
    Q_OBJECT

signals:
    void sendStartTime(double);
    void sendStopTime(double);
    void sendStartFreq(double);
    void sendStopFreq(double);
    void sendPSDType(PSDType);

public:
    explicit PSDInfo(QWidget *parent = nullptr);
    ~PSDInfo();

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_comboBox_currentIndexChanged(int index);

    void on_lineEdit_3_editingFinished();

    void on_lineEdit_4_editingFinished();

private:
    Ui::PSDInfo *ui;
};

#ifndef PSDINFO_H
#define PSDINFO_H

#include <QDialog>

namespace Ui {
class PSDInfo;
}

enum PSD_Type { EMPTY, Linear, Log };

class PSDInfo : public QDialog
{
    Q_OBJECT

signals:
    void sendStartTime(double);
    void sendStopTime(double);
    void sendStartFreq(double);
    void sendStopFreq(double);
    void sendPSDType(PSD_Type);

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

#endif // PSDINFO_H

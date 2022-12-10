#pragma once
#include <QDialog>
#include <QMessageBox>

namespace Ui {
class WignerInfo;
}

class WignerInfo : public QDialog
{
    Q_OBJECT

public:
    explicit WignerInfo(QWidget *parent = nullptr);
    ~WignerInfo();

signals:
    void sendBeginTime(double);
    void sendEndTime(double);
    void sendChannelName(QString);

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_3_editingFinished();

private:
    Ui::WignerInfo *ui;
};

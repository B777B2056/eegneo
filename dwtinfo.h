#ifndef DWTINFO_H
#define DWTINFO_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class DwtInfo;
}

class DwtInfo : public QDialog
{
    Q_OBJECT

public:
    explicit DwtInfo(QWidget *parent = nullptr);
    ~DwtInfo();

signals:
    void sendFreqMin(double);
    void sendFreqMax(double);
    void sendChannel(QString);

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_3_editingFinished();

private:
    Ui::DwtInfo *ui;
};

#endif // DWTINFO_H

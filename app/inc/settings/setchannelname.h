#pragma once
#include <cstddef>
#include <QDialog>

namespace Ui {
class setchannelname;
}

class SetChannelName : public QDialog
{
    Q_OBJECT

public:
    explicit SetChannelName(std::size_t channelNum, QWidget *parent = nullptr);
    ~SetChannelName();

    QString *names;

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_3_editingFinished();

    void on_lineEdit_4_editingFinished();

    void on_lineEdit_5_editingFinished();

    void on_lineEdit_6_editingFinished();

    void on_lineEdit_7_editingFinished();

    void on_lineEdit_8_editingFinished();

    void on_lineEdit_11_editingFinished();

    void on_lineEdit_15_editingFinished();

    void on_lineEdit_9_editingFinished();

    void on_lineEdit_13_editingFinished();

    void on_lineEdit_16_editingFinished();

    void on_lineEdit_14_editingFinished();

    void on_lineEdit_10_editingFinished();

    void on_lineEdit_12_editingFinished();

private:
    Ui::setchannelname *ui;
};

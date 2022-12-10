#pragma once
#include <QDialog>
#include <QMessageBox>

namespace Ui {
class p300oddballsetting;
}

class p300oddballsetting : public QDialog
{
    Q_OBJECT

public:
    explicit p300oddballsetting(QWidget *parent = nullptr);
    ~p300oddballsetting();

    double freq_2;
    int num_img;
    int crossDurationTime, numDurationTime, blankDurationTimeDown, blankDurationTimeUp;

private slots:
    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_4_editingFinished();

    void on_lineEdit_5_editingFinished();

    void on_lineEdit_6_editingFinished();

    void on_lineEdit_3_editingFinished();

private:
    Ui::p300oddballsetting *ui;
};

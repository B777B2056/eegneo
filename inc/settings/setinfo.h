#pragma once
#include <QDialog>
#include <QCloseEvent>
#include "utils/enum.h"

namespace Ui {
class set_info;
}

class SetInfo : public QDialog
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event) { event->accept(); mainwindow->~QWidget(); }

public:
    explicit SetInfo(QWidget *parent = nullptr);
    ~SetInfo();
    void setMainWindow(QWidget *mainwindow) { this->mainwindow = mainwindow; }
    void getInfo(QString& num_info, QString& date_info, QString& other_info, QString& exp_name, int& motange_num, BoardType& b);

private slots:
    void on_num_editingFinished();
    void on_date_editingFinished();
    void on_others_editingFinished();
    void on_exp_name_editingFinished();
    void on_comboBox_currentIndexChanged(int index);
    void on_comboBox_2_currentIndexChanged(int index);

private:
    QWidget *mainwindow;
    int motange_num;
    BoardType btype;
    QString num_info, date_info, other_info, exp_name;  // 被试信息

    Ui::set_info *ui;
};

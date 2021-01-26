#ifndef SETCHANNELNAME_H
#define SETCHANNELNAME_H

#include <QDialog>

namespace Ui {
class setchannelname;
}

class SetChannelName : public QDialog
{
    Q_OBJECT

public:
    explicit SetChannelName(QWidget *parent = nullptr);
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

private:
    Ui::setchannelname *ui;
};

#endif // SETCHANNELNAME_H

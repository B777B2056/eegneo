#pragma once
#include <QDialog>

namespace Ui {
class SetEventChannel;
}

class SetEventChannel : public QDialog
{
    Q_OBJECT

public:
    explicit SetEventChannel(QWidget *parent = nullptr);
    ~SetEventChannel();
    QString eventChannel;

private slots:
    void on_lineEdit_editingFinished();

private:
    Ui::SetEventChannel *ui;
};

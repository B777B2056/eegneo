#pragma once
#include <QDialog>

namespace Ui {
class ChooseCom;
}

class ChooseCom : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseCom(QWidget *parent = nullptr);
    ~ChooseCom();
    void getCom(QString& com);

private slots:
    void on_lineEdit_editingFinished();

private:
    QString _com;
    Ui::ChooseCom *ui;
};

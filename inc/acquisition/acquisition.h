#pragma once
#include <QWidget>

namespace Ui {
class acquisition;
}

class acquisition : public QWidget
{
    Q_OBJECT

public:
    explicit acquisition(QWidget *parent = nullptr);
    ~acquisition();

private:
    Ui::acquisition *ui;
};

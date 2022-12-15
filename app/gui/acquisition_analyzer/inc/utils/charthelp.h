#pragma once
#include <QWidget>
#include <QChart>

namespace Ui {
class ChartHelp;
}

class ChartHelp : public QWidget
{
    Q_OBJECT

public:
    explicit ChartHelp(QWidget *parent = nullptr);
    ~ChartHelp();
    Ui::ChartHelp *ui;
};

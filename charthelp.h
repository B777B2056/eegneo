#ifndef CHARTHELP_H
#define CHARTHELP_H

#include <QWidget>
#include <QChart>

using namespace QtCharts;

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

#endif // CHARTHELP_H

#ifndef ACQUISITION_H
#define ACQUISITION_H

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

#endif // ACQUISITION_H

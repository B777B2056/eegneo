#ifndef MAINBACKGROUND_H
#define MAINBACKGROUND_H

#include <QWidget>

namespace Ui {
class Background;
}

class Background : public QWidget
{
    Q_OBJECT

public:
    explicit Background(QWidget *parent = nullptr);
    ~Background();

private:
    Ui::Background *ui;
};

#endif // MAINBACKGROUND_H

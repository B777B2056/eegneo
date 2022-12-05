#pragma once
#include <QWidget>

namespace Ui {
class BackgroundWindow;
}

class BackgroundWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BackgroundWindow(QWidget *parent = nullptr);
    ~BackgroundWindow();

private:
    Ui::BackgroundWindow *ui;
};

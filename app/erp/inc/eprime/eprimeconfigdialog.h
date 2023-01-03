#pragma once
#include <QDialog>
#include "eprime/eprime.h"

namespace Ui {
class EPrimeConfigDialog;
}

class EPrimeConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EPrimeConfigDialog(QWidget *parent = nullptr);
    ~EPrimeConfigDialog();

    std::shared_ptr<eegneo::erp::EPrimeProxy> proxy() { return this->mEProxy_; }

private:
    std::shared_ptr<eegneo::erp::EPrimeProxy> mEProxy_;
    Ui::EPrimeConfigDialog *ui;
};

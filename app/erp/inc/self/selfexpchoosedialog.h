#pragma once
#include <QDialog>
#include <memory>

namespace Ui {
class SelfExpChooseDialog;
}

class QMainWindow;

class SelfExpChooseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelfExpChooseDialog(QWidget *parent = nullptr);
    ~SelfExpChooseDialog();

    std::shared_ptr<QMainWindow> runningExp() { return this->mExp_; }

private:
    std::shared_ptr<QMainWindow> mExp_;
    Ui::SelfExpChooseDialog *ui;
};

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include <QWidget>
#include "self/selfexpchoosedialog.h"
#include "eprime/eprimeconfigdialog.h"

int main(int argc, char *argv[])
{   
    QApplication app(argc, argv);

    std::shared_ptr<QMainWindow> selfExp;
    std::shared_ptr<eegneo::erp::EPrimeProxy> proxy;
    auto reply = QMessageBox::question(nullptr, QObject::tr("eegneo"), "使用系统自带实验？", QMessageBox::Ok | QMessageBox::No);
    if (QMessageBox::Ok == reply)
    {
        SelfExpChooseDialog dialog;
        if(int rec = dialog.exec(); QDialog::Accepted == rec)
        {
            selfExp = dialog.runningExp();
            selfExp->show();
        }
        else
        {
            return 0;
        }
    }
    else
    {
        EPrimeConfigDialog dialog;
        if(int rec = dialog.exec(); QDialog::Accepted == rec)
        {
            proxy = dialog.proxy();
            if (!proxy->open())
            {
                QMessageBox::critical(nullptr, QObject::tr("错误"), "端口打开失败", QMessageBox::Ok);
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    return app.exec();
}
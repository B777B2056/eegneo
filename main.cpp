#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include "setinfo.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    int channel_num = -1;
    QString participantNum, date, others, expName;
    SetInfo *siw = new SetInfo;
Retry:
    int rec = siw->exec();
    siw->getInfo(participantNum, date, others, expName, channel_num);
    if(rec == QDialog::Accepted){
        if(participantNum.isEmpty() || date.isEmpty() || expName.isEmpty() || !channel_num){
            /*被试信息必须项缺失，弹出错误信息后返回*/
            QMessageBox::StandardButton reply;
            reply = QMessageBox::critical(siw, siw->tr("错误"),
                                            "被试信息缺失！\n请检查被试编号、日期与实验名称。",
                                            QMessageBox::Retry | QMessageBox::Abort);
            if (reply == QMessageBox::Abort){
                siw->close();
            }else{
                goto Retry;
            }
        }
    }else{
        siw->close();
    }
    delete siw;
    MainWindow w(participantNum, date, others, expName, channel_num);
    w.show();

    return a.exec();
}


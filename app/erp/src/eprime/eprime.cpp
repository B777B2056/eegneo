#include "eprime/eprime.h"
#include <cstdlib>
#include <QMessageBox>
#include "common/common.h"
#include "utils/config.h"

namespace eegneo
{
    namespace erp
    {
        EPrimeProxy::EPrimeProxy()
            : BaseIpc()
        {
            QObject::connect(&this->mSerialPort_, &QSerialPort::readyRead, [this]()->void
            {
                this->handleRead();
            });

            if (!this->mSerialPort_.open(QSerialPort::ReadOnly))
            {
                if (QMessageBox::Ok == QMessageBox::critical(nullptr, QObject::tr("错误"), "端口打开失败", QMessageBox::Ok))
                {
                    std::exit(0);
                }
            }
        }

        EPrimeProxy::~EPrimeProxy()
        {
            this->mSerialPort_.close();
        }

        void EPrimeProxy::handleRead()
        {
            if (!this->mSerialPort_.bytesAvailable())   return;
            this->sendMarker(this->mSerialPort_.readAll().data());
        }
    }   // namespace erp
}   // namespace eegneo

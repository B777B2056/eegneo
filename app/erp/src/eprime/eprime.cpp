#include "eprime/eprime.h"
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
        }

        EPrimeProxy::~EPrimeProxy()
        {
            
        }

        void EPrimeProxy::handleRead()
        {
            // TODO
            this->sendMarker("hello");
        }
    }   // namespace erp
}   // namespace eegneo

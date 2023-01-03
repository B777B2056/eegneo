#pragma once
#include <QSerialPort>
#include "baseipc.h"

namespace eegneo
{
    namespace erp
    {
        class EPrimeProxy : public BaseIpc
        {
        public:
            EPrimeProxy();
            ~EPrimeProxy();

            QSerialPort& serialPort() { return this->mSerialPort_; }

        private:
            QSerialPort mSerialPort_;

            void handleRead();
        };
    }   // namespace erp
}   // namespace eegneo

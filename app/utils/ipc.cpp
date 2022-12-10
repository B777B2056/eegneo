#include "ipc.h"

#define ReadAndHandler(CMDType) \
    {   \
        if (CMDType##Cmd cmd; mChannel_->read((char*)&cmd, sizeof(cmd)))   \
        {   \
            this->mHandlers_[cmdHdr.id](&cmd);  \
        }   \
    }  

namespace eegneo
{
    namespace utils
    {
        IpcReader::IpcReader(QTcpSocket* channel)
            : mChannel_(channel)
        {
            QObject::connect(mChannel_, SIGNAL(readyRead()), this, SLOT(handleMsg()));
        }

        void IpcReader::handleMsg()
        {
            if (mChannel_->bytesAvailable() <= 0)
            {
                return;
            }
            CmdHeader cmdHdr;
            if (!mChannel_->read((char*)&cmdHdr, sizeof(cmdHdr)))
            {
                return;
            }
            switch (cmdHdr.id)
            {
            case CmdId::Rec:
                ReadAndHandler(Record);
                break;
            case CmdId::Filt:
                ReadAndHandler(Filt);
                break;
            case CmdId::Shutdown:
                ReadAndHandler(Shutdown);
                break;
            default:
                break;
            }
        }

        IpcWriter::IpcWriter(QTcpSocket* channel)
            : mChannel_(channel)
        {
            
        }
    }   // namespace utils
}   // namespace eegneo
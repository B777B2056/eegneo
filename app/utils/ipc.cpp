#include "ipc.h"

#define ReadAndHandler(CMDType) \
    do  \
    {   \
        if (CMDType##Cmd cmd; this->readBytes((char*)&cmd, sizeof(cmd)))   \
        {   \
            this->mHandlers_[cmdHdr.id](&cmd);  \
        }   \
    }   \
    while (0)

namespace eegneo
{
    namespace utils
    {
        IpcReader::IpcReader(QTcpSocket* channel)
            : mChannel_(channel)
        {
            QObject::connect(mChannel_, &QTcpSocket::readyRead, [this]()->void{ this->handleMsg(); });
        }

        void IpcReader::handleMsg()
        {
            if (!mChannel_->bytesAvailable())
            {
                return;
            }
            CmdHeader cmdHdr;
            if (!this->readBytes((char*)&cmdHdr, sizeof(cmdHdr)))
            {
                return;
            }
            switch (cmdHdr.id)
            {
            case CmdId::Record:
                ReadAndHandler(Record);
                break;
            case CmdId::Filt:
                ReadAndHandler(Filt);
                break;
            case CmdId::Shutdown:
                ReadAndHandler(Shutdown);
                break;
            case CmdId::Marker:
                ReadAndHandler(Marker);
                break;
            default:
                break;
            }
        }

        bool IpcReader::readBytes(char* buf, std::uint16_t bytesLength)
        {   
            std::uint16_t bytesReceived = 0;    
            do  
            {   
                int t = mChannel_->read(buf + bytesReceived, bytesLength - bytesReceived);  
                if (-1 == t) return false;
                bytesReceived += t; 
            } while (bytesReceived < bytesLength);   
            return true;
        }

        IpcWriter::IpcWriter(QTcpSocket* channel)
            : mChannel_(channel)
        {
            
        }
    }   // namespace utils
}   // namespace eegneo
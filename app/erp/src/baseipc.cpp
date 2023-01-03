#include "baseipc.h"
#include <QMessageBox>
#include "common/common.h"
#include "utils/config.h"

namespace eegneo
{
    namespace erp
    {
        BaseIpc::BaseIpc()
        {
            // 采集软件所在电脑的Ip地址和端口号
            auto& config = eegneo::utils::ConfigLoader::instance();
            auto ip = config.get<std::string>("ERP", "AcquisitionIpAddr");
            auto port = config.get<std::uint16_t>("IpcServerIpPort");
            this->mIpc_ = new eegneo::utils::IpcClient(eegneo::SessionId::ERPSession, ip.c_str(), port);
            this->mIpc_->setErrorCallback([this](QAbstractSocket::SocketError err)->void
            {
                switch (err)
                {
                case QAbstractSocket::ConnectionRefusedError:
                    QMessageBox::critical(nullptr, QObject::tr("错误"), "连接采集平台被拒绝", QMessageBox::Ok);
                    break;
                case QAbstractSocket::HostNotFoundError:
                    QMessageBox::critical(nullptr, QObject::tr("错误"), "采集平台地址错误，请检查IP地址配置", QMessageBox::Ok);
                    break;
                case QAbstractSocket::SocketTimeoutError:
                    QMessageBox::critical(nullptr, QObject::tr("错误"), "连接采集平台失败", QMessageBox::Ok);
                    break;
                case QAbstractSocket::NetworkError:
                    QMessageBox::critical(nullptr, QObject::tr("错误"), "网络错误，请检查网络是否正常", QMessageBox::Ok);
                    break;
                }
            });
        }

        BaseIpc::~BaseIpc()
        {
            delete this->mIpc_;
        }

        void BaseIpc::sendMarker(const char* msg)
        {
            eegneo::MarkerCmd cmd;
            ::memcpy(cmd.msg, msg, std::strlen(msg));
            mIpc_->sendCmd(cmd);
        }
    }   // namespace erp
}   // namespace eegneo

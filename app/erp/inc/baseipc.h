#pragma once
#include "utils/ipc.h"

namespace eegneo
{
    namespace erp
    {
        class BaseIpc
        {
        public:
            BaseIpc();
            ~BaseIpc();

            void sendMarker(const char* msg);

        protected:
            eegneo::utils::IpcClient* mIpc_;
        };
    }   // namespace erp
}   // namespace eegneo

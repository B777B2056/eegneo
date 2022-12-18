#include "config.h"
#include <QFile>
#include <QTextStream>

namespace eegneo
{
    namespace utils
    {
        ConfigLoader::ConfigLoader()
        {
            if (QFile file(":/config/resource/config.json"); file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                this->mConfigJson_ = nlohmann::json::parse(QTextStream(&file).readAll().toStdString());
            }
        }

        ConfigLoader& ConfigLoader::instance()
        {
            static ConfigLoader inst;
            return inst;
        }
    }   // namepsace utils
}   // namespace eegneo

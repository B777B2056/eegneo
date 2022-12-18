#pragma once
#include "third/json/json.hpp"

namespace eegneo
{
    namespace utils
    {
        class ConfigLoader
        {
        public:
            static ConfigLoader& instance();

            template<typename T>
            T get(const char* key) const
            {
                return this->mConfigJson_.at(key).get<T>();
            }

            template<typename T, typename ... Args>
            T get(const char* keyN, Args&&... keys) const
            {
                return this->getHelper<T>(this->mConfigJson_.at(keyN), keys...);
            }

        private:
            nlohmann::json mConfigJson_;

            ConfigLoader();

            template<typename T>
            T getHelper(const nlohmann::json& j, const char* key) const
            {
                return j.at(key).get<T>();
            }

            template<typename T, typename ... Args>
            T getHelper(const nlohmann::json& j, const char* keyN, Args&&... keys) const
            {
                return this->getHelper<T>(j.at(keyN), keys...);
            }
        };
    }   // namespace utils
}   // namespace eegneo

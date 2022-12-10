#pragma once
#include <cstddef>
#include <vector>

namespace eegneo
{
    namespace utils
    {
        class EEGFileReader
        {
        public:
            EEGFileReader(const char* filePath);
            ~EEGFileReader();

            const std::vector<double>& atChannel(std::size_t channel) const { return mData_.at(channel); }

        private:
            std::vector<std::vector<double>> mData_;
        };

        class EDFReader : public EEGFileReader
        {
        public:
            using EEGFileReader::EEGFileReader;
        };

        class EEGFileWriter
        {

        };
    }   // namespace utils
}   // namespace eegneo

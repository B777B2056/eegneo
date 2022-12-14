#pragma once
#include <cstddef>
#include <vector>
#include <string>
#include "common/common.h"

namespace eegneo
{
    namespace utils
    {
        struct EEGAnnotation
        {
            std::int64_t onSetTimeMs;   // 开始时刻              
            // std::string duration;    // 持续时间            
            std::string description;    // 事件描述
        };

        class EEGFileReader
        {
        public:
            EEGFileReader(const char* filePath);
            virtual ~EEGFileReader() {}
            
            std::size_t channelNum() const { return mChannelNum_; }
            std::uint64_t sampleFreqencyHz() const { return mSampleFreqencyHz_; }
            const char* channelName(std::size_t idx) const { return mChannelNames_.at(idx).c_str(); }
            const std::vector<double>& channel(std::size_t idx) const { return mData_.at(idx); }
            const std::vector<EEGAnnotation>& annotation() const { return mAnnotations_; }

        protected:
            std::size_t mChannelNum_;
            std::uint64_t mSampleFreqencyHz_;
            std::vector<EEGAnnotation> mAnnotations_;  
            std::vector<std::vector<double>> mData_;
            std::vector<std::string> mChannelNames_;

            virtual void loadFile(const char* filePath) = 0;
        };

        class EDFReader : public EEGFileReader
        {
        public:
            EDFReader(const char* filePath);
            ~EDFReader();

        private:
            void loadFile(const char* filePath) override;
        };

        class EEGFileWriter
        {
        public:
            EEGFileWriter(const char* filePath, std::size_t channelNum);
            virtual ~EEGFileWriter() {}

            virtual void setSampleFreqencyHz(std::uint64_t sampleFreqencyHz) = 0;
            virtual void setChannelName(std::size_t i, const char* name) = 0;
            virtual void saveRecordData() = 0;
            virtual void saveAnnotation() = 0;

        protected:
            std::size_t mChannelNum_;
            std::uint64_t mSampleFreqencyHz_;
        };

        class EDFWritter : public EEGFileWriter
        {
        public:
            EDFWritter(const char* filePath, std::size_t channelNum, EDFFileType type);
            ~EDFWritter();

            void setSampleFreqencyHz(std::uint64_t sampleFreqencyHz) override;
            void setChannelName(std::size_t i, const char* name) override;
            void saveRecordData() override;
            void saveAnnotation() override;
            
        private:
            int mFp_;   // 文件句柄
            void writeChannelOneSecond(const std::vector<double>& channelData);
            void writeAnnotations(const std::vector<EEGAnnotation>& annotations);
        };
    }   // namespace utils
}   // namespace eegneo

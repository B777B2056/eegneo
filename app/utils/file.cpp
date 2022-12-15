#include "file.h"
#include <fstream>
#include <iostream>  
extern "C" 
{ 
#include "third/edf/edflib.h" 
}

namespace eegneo
{
    namespace utils
    {
        EEGFileReader::EEGFileReader(const char* filePath)
            : mChannelNum_(0), mSampleFreqencyHz_(0)
        {

        }

        EDFReader::EDFReader(const char* filePath)
            : EEGFileReader(filePath)
        {
            this->loadFile(filePath);
        }

        EDFReader::~EDFReader()
        {

        }

        void EDFReader::loadFile(const char* filePath)
        {
            struct edf_hdr_struct edfhdr;
            int fp = ::edfopen_file_readonly(filePath, &edfhdr, EDFLIB_READ_ANNOTATIONS);
            if (-1 == fp)
            {
                // TODO: ERROR HANDLE
                return;
            }
            // 读取通道数
            this->mChannelNum_ = edfhdr.edfsignals; 
            std::cout << "channel count: " << this->mChannelNum_ << "\n"; 
            this->mData_.resize(this->mChannelNum_);
            this->mChannelNames_.resize(this->mChannelNum_);
            //读取采样率
            this->mSampleFreqencyHz_ = edfhdr.signalparam[0].smp_in_datarecord/(edfhdr.datarecord_duration/10000000);
            std::cout << this->mSampleFreqencyHz_ << "Hz\n";
            // 读取数据
            for (int i = 0; i < edfhdr.edfsignals; ++i)
            {
                this->mData_[i].resize(edfhdr.datarecords_in_file);
                if (-1 == ::edfread_physical_samples(fp, i, (int)edfhdr.datarecords_in_file, this->mData_[i].data()))
                {
                    this->mData_.clear();
                    // TODO: ERROR HANDLE
                    return;
                }
                this->mChannelNames_[i] = edfhdr.signalparam[i].label;
            }
            // 读取事件与事件描述
            struct edf_annotation_struct annot;
            this->mAnnotations_.resize(edfhdr.annotations_in_file);
            for (int i = 0; i < edfhdr.annotations_in_file; ++i)
            {
                if (-1 == ::edf_get_annotation(fp, i, &annot))
                {
                    this->mAnnotations_.clear();
                    // TODO: ERROR HANDLE
                    return;
                }
                this->mAnnotations_[i] = EEGAnnotation{annot.onset, annot.annotation};
            }
            // 关闭文件
            ::edfclose_file(fp);
        }

        EEGFileWriter::EEGFileWriter(const char* filePath, std::size_t channelNum)
            : mChannelNum_(channelNum), mSampleFreqencyHz_(0)
        {

        }

        static std::fstream debug_file{"E:/jr/eegneo/debug.txt", std::ios::out};

        EDFWritter::EDFWritter(const char* filePath, std::size_t channelNum, EDFFileType type)
            : EEGFileWriter(filePath, channelNum)
        {
            this->mFp_ = ::edfopen_file_writeonly(filePath, ((EDFFileType::EDF == type) ? EDFLIB_FILETYPE_EDFPLUS : EDFLIB_FILETYPE_BDFPLUS), static_cast<int>(channelNum));
            debug_file << "fp = " << mFp_ << std::endl;
        }

        EDFWritter::~EDFWritter()
        {
            if (-1 == this->mFp_)   return;
            ::edfclose_file(this->mFp_);
        }

        void EDFWritter::setSampleFreqencyHz(std::uint64_t sampleFreqencyHz)
        {
            if (-1 == this->mFp_)   return;
            for (std::size_t i = 0; i < this->mChannelNum_; ++i)
            {
                ::edf_set_samplefrequency(this->mFp_, static_cast<int>(i), static_cast<int>(sampleFreqencyHz));
                ::edf_set_physical_maximum(this->mFp_, static_cast<int>(i), 2500000.0); //设置物理最大值
                ::edf_set_physical_minimum(this->mFp_, static_cast<int>(i), -2500000.0);//设置物理最小值
                ::edf_set_digital_maximum(this->mFp_, static_cast<int>(i), 32767);    //设置最大值
                ::edf_set_digital_minimum(this->mFp_, static_cast<int>(i), -32768);   //设置最小值
                ::edf_set_physical_dimension(this->mFp_, static_cast<int>(i), "uV");    // 设置物理单位
            }
            this->mSampleFreqencyHz_ = sampleFreqencyHz;
        }

        void EDFWritter::setChannelName(std::size_t i, const char* name)
        {
            if (!this->mFp_)   return;
            ::edf_set_label(this->mFp_, static_cast<int>(i), name);
        }

        void EDFWritter::writeChannelOneSecond(const std::vector<double>& channelData)
        {
            if (-1 == this->mFp_)   return;
            ::edf_blockwrite_physical_samples(this->mFp_, channelData.data());
        }

        void EDFWritter::writeAnnotations(const std::vector<EEGAnnotation>& annotations)
        {
            if (-1 == this->mFp_)   return;
            for (const auto& annotation : annotations)
            {
                ::edfwrite_annotation_utf8(this->mFp_, annotation.onSetTimeMs * 10, -1, annotation.description.c_str());
            }
        }

        void EDFWritter::saveRecordData()
        {
            if (!this->mChannelNum_ || !this->mSampleFreqencyHz_)
            {
                return;
            }
            // 打开缓存文件
            std::fstream dataCacheFile{DATA_FILE_PATH, std::ios::in | std::ios::binary};
            if (!dataCacheFile.is_open())
            {
                return;
            }
            // 读入数据缓存文件（读1秒数据后立即写入，直至读完文件）
            std::vector<double> data;
            data.resize(this->mChannelNum_ * this->mSampleFreqencyHz_, 0.0);
            dataCacheFile.seekg(0, std::ios::beg);
            while (!dataCacheFile.eof())
            {
                dataCacheFile.read(reinterpret_cast<char*>(data.data()), data.size() * sizeof(double));
                this->writeChannelOneSecond(data);  // 写入目标格式文件
            }
        }

        void EDFWritter::saveAnnotation()
        {
            if (!this->mChannelNum_ || !this->mSampleFreqencyHz_)
            {
                return;
            }
            // 打开缓存文件
            std::fstream eventCacheFile{EVENT_FILE_PATH, std::ios::in};
            if (!eventCacheFile.is_open())
            {
                return;
            }
            eventCacheFile.seekg(0, std::ios::beg);
            // 读入事件缓存文件
            std::vector<utils::EEGAnnotation> annotations;
            for (std::string line; std::getline(eventCacheFile, line); )
            {
                auto pos = line.find_first_of(':');
                if (pos == std::string::npos)   return;
                std::int64_t onSetTimeMs = (std::int64_t)(std::stoll(line.substr(0, pos)) * (1000.0 / (double)this->mSampleFreqencyHz_));
                annotations.emplace_back(utils::EEGAnnotation{onSetTimeMs, line.substr(pos+1, line.size())});
            }
            // 写入目标格式文件
            if (!annotations.empty())
            {
                this->writeAnnotations(annotations);
            }
        }
    }
}   // namespace eegneo

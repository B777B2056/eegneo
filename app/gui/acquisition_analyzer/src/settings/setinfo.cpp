#include "settings/setinfo.h"
#include "ui_setinfo.h"

SetInfo::SetInfo(QWidget *parent)
    : QDialog(parent)
    , mSubjectNum_(""), mChannelNum_(0), mSampleRate_(0)
    , ui(new Ui::set_info)
{
    ui->setupUi(this);
}

SetInfo::~SetInfo()
{
    delete ui;
}

void SetInfo::on_num_editingFinished()
{
    this->mSubjectNum_ = ui->num->text();
}

void SetInfo::on_comboBox_currentIndexChanged(int index)
{
    if (1 == index) this->mChannelNum_ = 8LL;
    else if (2 == index) this->mChannelNum_ = 16LL;
    else this->mChannelNum_ = 32LL;
}

void SetInfo::on_comboBox_2_currentIndexChanged(int index)
{
    if (1 == index) this->mSampleRate_ = 128LL;
    else if (2 == index) this->mSampleRate_ = 256LL;
    else if (3 == index) this->mSampleRate_ = 512LL;
    else this->mSampleRate_ = 1024LL;
}

#pragma once
#include <cstddef>
#include <QDialog>
#include <QString>

namespace Ui {
class set_info;
}

class SetInfo : public QDialog
{
    Q_OBJECT

public:
    explicit SetInfo(QWidget *parent = nullptr);
    ~SetInfo();

    QString subjectNum() const { return mSubjectNum_; }
    std::size_t channelNum() const { return mChannelNum_; }
    std::size_t sampleRate() const { return mSampleRate_; }

private slots:
    void on_num_editingFinished();
    void on_comboBox_currentIndexChanged(int index);
    void on_comboBox_2_currentIndexChanged(int index);

private:
    QString mSubjectNum_;
    std::size_t mChannelNum_;
    std::size_t mSampleRate_;
    Ui::set_info *ui;
};

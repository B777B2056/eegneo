#include "self/selfexpchoosedialog.h"
#include "ui_selfexpchoosedialog.h"
#include "self/p300.h"

SelfExpChooseDialog::SelfExpChooseDialog(QWidget *parent)
    : QDialog(parent)
    , mExp_(nullptr)
    , ui(new Ui::SelfExpChooseDialog)
{
    ui->setupUi(this);
    QObject::connect(ui->comboBox, qOverload<int>(&QComboBox::activated), [this](int index)->void
    {
        if (1 == index)
        {
            this->mExp_ = std::make_shared<ErpP300OddballWindow>();
        }
    });
}

SelfExpChooseDialog::~SelfExpChooseDialog()
{
    delete ui;
}

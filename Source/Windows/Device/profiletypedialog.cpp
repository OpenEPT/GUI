#include "profiletypedialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QIcon>

ProfileTypeDialog::ProfileTypeDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Profile type");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    typeCombo = new QComboBox(this);
    typeCombo->addItem(QIcon(":/images/NewSet/battery.png"), "Battery Analysis");
    typeCombo->addItem(QIcon(":/images/main.png"), "Consumption Analysis");
    typeCombo->addItem("Other (Default)");

    batteryNameEdit = new QLineEdit(this);
    batteryNameEdit->setToolTip("Name or model of the battery under test");
    batteryCapacityEdit = new QLineEdit(this);
    batteryCapacityEdit->setToolTip("Declared capacity of the battery in mAh");

    batteryNameLabel = new QLabel("Battery name", this);
    batteryCapacityLabel = new QLabel("Declared capacity [mAh]", this);

    formLayout->addRow("Analysis type", typeCombo);
    formLayout->addRow(batteryNameLabel, batteryNameEdit);
    formLayout->addRow(batteryCapacityLabel, batteryCapacityEdit);

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeChanged()));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileTypeDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    resize(320, 160);
    onTypeChanged();
}

void ProfileTypeDialog::onTypeChanged()
{
    bool battery = (typeCombo->currentIndex() == 0);

    batteryNameLabel->setVisible(battery);
    batteryNameEdit->setVisible(battery);
    batteryCapacityLabel->setVisible(battery);
    batteryCapacityEdit->setVisible(battery);
}

void ProfileTypeDialog::onAccept()
{
    if(typeCombo->currentIndex() == 0)
    {
        if(batteryNameEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, "Profile type", "Please enter the battery name");
            return;
        }

        bool ok;
        double capacity = batteryCapacityEdit->text().toDouble(&ok);
        if(!ok || capacity <= 0)
        {
            QMessageBox::warning(this, "Profile type", "Please enter a valid declared capacity in mAh");
            return;
        }
    }

    accept();
}

void ProfileTypeDialog::setValues(const QString &type, const QString &batteryName, const QString &batteryCapacity)
{
    if(type.compare("Battery", Qt::CaseInsensitive) == 0) typeCombo->setCurrentIndex(0);
    else if(type.compare("Consumption", Qt::CaseInsensitive) == 0) typeCombo->setCurrentIndex(1);
    else typeCombo->setCurrentIndex(2);

    batteryNameEdit->setText(batteryName);
    batteryCapacityEdit->setText(batteryCapacity);

    onTypeChanged();
}

QString ProfileTypeDialog::analysisType() const
{
    if(typeCombo->currentIndex() == 0) return "Battery";
    if(typeCombo->currentIndex() == 1) return "Consumption";
    return "Other";
}

QString ProfileTypeDialog::batteryName() const
{
    return batteryNameEdit->text().trimmed();
}

QString ProfileTypeDialog::batteryCapacity() const
{
    return batteryCapacityEdit->text().trimmed();
}

#ifndef PROFILETYPEDIALOG_H
#define PROFILETYPEDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

class ProfileTypeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit            ProfileTypeDialog(QWidget *parent = nullptr);

    void                setValues(const QString &type, const QString &batteryName, const QString &batteryCapacity);
    QString             analysisType() const;
    QString             batteryName() const;
    QString             batteryCapacity() const;

private slots:
    void                onTypeChanged();
    void                onAccept();

private:
    QComboBox          *typeCombo;
    QLineEdit          *batteryNameEdit;
    QLineEdit          *batteryCapacityEdit;
    QLabel             *batteryNameLabel;
    QLabel             *batteryCapacityLabel;
};

#endif // PROFILETYPEDIALOG_H

#include "chargerwnd.h"
#include "device.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QTextEdit>
#include <QFontDatabase>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QIcon>
#include <QSize>

#define CHARGER_ICON_BTN_SIZE   32
#define CHARGER_ICON_SIZE       24

static const char* CHARGER_ICON_BTN_STYLE = R"(
QPushButton { background: transparent; border: none; }
QPushButton:hover   { background-color: rgba(255,255,255,30); }
QPushButton:pressed { background-color: rgba(255,255,255,60); }
QToolTip { background-color: white; color: black; border: 1px solid #ccc; }
)";

/* BQ25180 registers and bit fields (see datasheet SLUSE99C) */
#define BQ25180_REG_STAT0           0x00    /* Charger status           */
#define BQ25180_REG_CHARGERCTRL1    0x06    /* BUVLO[5:3]               */
#define BQ25180_REG_IC_CTRL         0x07    /* VLOWV_SEL[6]             */

#define BUVLO_MASK                  0x38    /* bits 5-3                  */
#define BUVLO_SHIFT                 3
#define VLOWV_MASK                  0x40    /* bit 6                     */
#define VLOWV_SHIFT                 6

ChargerWnd::ChargerWnd(Device* device, QWidget *parent) :
    QWidget(parent),
    m_device(device)
{
    buildUi();

    connect(readPusb,           SIGNAL(clicked()), this, SLOT(onReadClicked()));
    connect(setPusb,            SIGNAL(clicked()), this, SLOT(onSetClicked()));
    connect(storePusb,          SIGNAL(clicked()), this, SLOT(onStoreClicked()));
    connect(startPusb,          SIGNAL(clicked()), this, SLOT(onStartChargingClicked()));
    connect(stopPusb,           SIGNAL(clicked()), this, SLOT(onStopChargingClicked()));
    connect(applyChemistryPusb, SIGNAL(clicked()), this, SLOT(onApplyChemistryClicked()));
    connect(eepromReadPusb,   SIGNAL(clicked()), this, SLOT(onEepromReadClicked()));
    connect(eepromWritePusb,  SIGNAL(clicked()), this, SLOT(onEepromWriteClicked()));
    connect(eepromFormatPusb, SIGNAL(clicked()), this, SLOT(onEepromFormatClicked()));
    connect(eepromImportPusb, SIGNAL(clicked()), this, SLOT(onEepromImportClicked()));
    connect(eepromExportPusb, SIGNAL(clicked()), this, SLOT(onEepromExportClicked()));

    /* Live charging-mode updates pushed by the firmware over the serial link. */
    if(m_device != nullptr)
        connect(m_device, SIGNAL(sigChargerModeObtained(QString)), this, SLOT(onChargerModeObtained(QString)));

    /* Populate current values from the device. */
    onReadClicked();
}

void ChargerWnd::buildUi()
{
    /* Everything lives inside a scroll area so the window never grows past the
     * screen; the top groups are laid out in two columns to keep it compact. */
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget*     content = new QWidget();
    QVBoxLayout* mainL   = new QVBoxLayout(content);

    /* ---- Device information (read-only) ---- */
    QGroupBox*  infoBox = new QGroupBox("Device information");
    QFormLayout* infoL  = new QFormLayout(infoBox);
    hwSerialEdit  = new QLineEdit(infoBox);  hwSerialEdit->setReadOnly(true);
    fwVersionEdit = new QLineEdit(infoBox);  fwVersionEdit->setReadOnly(true);
    infoL->addRow("HW serial:",  hwSerialEdit);
    infoL->addRow("FW version:", fwVersionEdit);

    /* ---- Live status ---- */
    QGroupBox*  statusBox = new QGroupBox("Status");
    QFormLayout* statusL  = new QFormLayout(statusBox);
    chargingModeLabel = new QLabel("—", statusBox);
    QFont modeFont = chargingModeLabel->font(); modeFont.setBold(true); chargingModeLabel->setFont(modeFont);
    stat0Label = new QLabel("—", statusBox);
    statusL->addRow("Charging mode:", chargingModeLabel);
    statusL->addRow("STAT0:",         stat0Label);

    /* ---- Charging parameters ---- */
    QGroupBox*  paramBox = new QGroupBox("Charging parameters");
    QFormLayout* paramL  = new QFormLayout(paramBox);

    termVoltageEdit = new QLineEdit(paramBox);
    termVoltageEdit->setValidator(new QDoubleValidator(3.5, 4.65, 2, termVoltageEdit));
    paramL->addRow("Termination voltage [V]:", termVoltageEdit);

    termCurrentCombo = new QComboBox(paramBox);
    termCurrentCombo->addItem("Disabled", 0);
    termCurrentCombo->addItem("5 %",      1);
    termCurrentCombo->addItem("10 %",     2);
    termCurrentCombo->addItem("20 %",     3);
    paramL->addRow("Termination current:", termCurrentCombo);

    chargeCurrentEdit = new QLineEdit(paramBox);
    chargeCurrentEdit->setValidator(new QIntValidator(5, 1000, chargeCurrentEdit));
    paramL->addRow("Charging current [mA]:", chargeCurrentEdit);

    maxCurrentCombo = new QComboBox(paramBox);
    maxCurrentCombo->addItem("50 mA",   0);
    maxCurrentCombo->addItem("100 mA",  1);
    maxCurrentCombo->addItem("200 mA",  2);
    maxCurrentCombo->addItem("300 mA",  3);
    maxCurrentCombo->addItem("400 mA",  4);
    maxCurrentCombo->addItem("500 mA",  5);
    maxCurrentCombo->addItem("700 mA",  6);
    maxCurrentCombo->addItem("1100 mA", 7);
    paramL->addRow("Max charging current:", maxCurrentCombo);

    /* ---- Protection thresholds ---- */
    QGroupBox*  protBox = new QGroupBox("Protection thresholds");
    QFormLayout* protL  = new QFormLayout(protBox);

    buvloCombo = new QComboBox(protBox);
    buvloCombo->addItem("3.0 V", 2);
    buvloCombo->addItem("2.8 V", 3);
    buvloCombo->addItem("2.6 V", 4);
    buvloCombo->addItem("2.4 V", 5);
    buvloCombo->addItem("2.2 V", 6);
    buvloCombo->addItem("2.0 V", 7);
    protL->addRow("Battery UVLO (falling):", buvloCombo);

    vlowvCombo = new QComboBox(protBox);
    vlowvCombo->addItem("3.0 V", 0);
    vlowvCombo->addItem("2.8 V", 1);
    protL->addRow("Precharge threshold (VLOWV):", vlowvCombo);

    /* ---- Battery chemistry preset ---- */
    QGroupBox*  chemBox = new QGroupBox("Battery chemistry preset");
    QHBoxLayout* chemL  = new QHBoxLayout(chemBox);
    chemistryCombo = new QComboBox(chemBox);
    chemistryCombo->addItem("Li-Ion / LiPo (4.2 V)");
    chemistryCombo->addItem("LiFePO4 (3.6 V)");
    applyChemistryPusb = new QPushButton("Apply preset", chemBox);
    chemL->addWidget(chemistryCombo);
    chemL->addWidget(applyChemistryPusb);

    /* ---- Two-column arrangement of the groups above ---- */
    QHBoxLayout* colsL  = new QHBoxLayout();
    QVBoxLayout* leftL  = new QVBoxLayout();
    QVBoxLayout* rightL = new QVBoxLayout();
    leftL->addWidget(infoBox);
    leftL->addWidget(paramBox);
    leftL->addStretch(1);
    rightL->addWidget(statusBox);
    rightL->addWidget(protBox);
    rightL->addWidget(chemBox);
    rightL->addStretch(1);
    colsL->addLayout(leftL);
    colsL->addLayout(rightL);
    mainL->addLayout(colsL);

    /* ---- Read / Set / Store (compact, right-aligned) ---- */
    QHBoxLayout* btnL = new QHBoxLayout();
    readPusb  = new QPushButton("Read");
    setPusb   = new QPushButton("Set");
    storePusb = new QPushButton("Store");
    readPusb->setFixedWidth(90);
    setPusb->setFixedWidth(90);
    storePusb->setFixedWidth(90);
    readPusb->setToolTip("Read current settings from the charger");
    setPusb->setToolTip("Apply settings now (volatile)");
    storePusb->setToolTip("Store settings to EEPROM (persist across power cycles)");
    btnL->addStretch(1);
    btnL->addWidget(readPusb);
    btnL->addWidget(setPusb);
    btnL->addWidget(storePusb);
    mainL->addLayout(btnL);

    /* ---- Charging control (compact, right-aligned) ---- */
    QGroupBox*  ctrlBox = new QGroupBox("Charging control");
    QHBoxLayout* ctrlL  = new QHBoxLayout(ctrlBox);
    startPusb = new QPushButton("Start charging", ctrlBox);
    stopPusb  = new QPushButton("Stop charging",  ctrlBox);
    startPusb->setFixedWidth(120);
    stopPusb->setFixedWidth(120);
    ctrlL->addStretch(1);
    ctrlL->addWidget(startPusb);
    ctrlL->addWidget(stopPusb);
    mainL->addWidget(ctrlBox);

    /* ---- EEPROM (AT24CS01) memory view ---- */
    QGroupBox*  eepromBox = new QGroupBox("EEPROM memory");
    QVBoxLayout* eepromL  = new QVBoxLayout(eepromBox);

    /* Header: icon buttons in the top-right, like the device memory view. */
    QHBoxLayout* eepromHeaderL = new QHBoxLayout();

    eepromReadPusb   = new QPushButton(eepromBox);
    eepromWritePusb  = new QPushButton(eepromBox);
    eepromFormatPusb = new QPushButton(eepromBox);
    eepromImportPusb = new QPushButton(eepromBox);
    eepromExportPusb = new QPushButton(eepromBox);

    struct IconBtn { QPushButton* b; const char* icon; const char* tip; };
    const IconBtn ebtns[] = {
        { eepromReadPusb,   ":/images/NewSet/upload.png",   "Read EEPROM content"    },
        { eepromWritePusb,  ":/images/NewSet/download.png", "Write EEPROM content"   },
        { eepromFormatPusb, ":/images/NewSet/format.png",   "Format (erase) EEPROM"  },
        { eepromImportPusb, ":/images/NewSet/import.png",   "Import image from PC"   },
        { eepromExportPusb, ":/images/NewSet/export.png",   "Export image to PC"     },
    };
    for(const IconBtn& e : ebtns)
    {
        e.b->setFixedSize(CHARGER_ICON_BTN_SIZE, CHARGER_ICON_BTN_SIZE);
        e.b->setIcon(QIcon(e.icon));
        e.b->setIconSize(QSize(CHARGER_ICON_SIZE, CHARGER_ICON_SIZE));
        e.b->setToolTip(e.tip);
        e.b->setStyleSheet(CHARGER_ICON_BTN_STYLE);
    }

    eepromHeaderL->addStretch();
    eepromHeaderL->addWidget(eepromReadPusb);
    eepromHeaderL->addWidget(eepromWritePusb);
    eepromHeaderL->addWidget(eepromFormatPusb);
    eepromHeaderL->addWidget(eepromImportPusb);
    eepromHeaderL->addWidget(eepromExportPusb);
    eepromL->addLayout(eepromHeaderL);

    eepromEdit = new QTextEdit(eepromBox);
    eepromEdit->setReadOnly(true);
    eepromEdit->setMinimumHeight(180);
    eepromEdit->setLineWrapMode(QTextEdit::NoWrap);
    eepromEdit->setFont(QFont("Courier New"));
    eepromL->addWidget(eepromEdit);

    mainL->addWidget(eepromBox);

    statusLabel = new QLabel("");
    mainL->addWidget(statusLabel);

    mainL->addStretch(1);

    scroll->setWidget(content);
    outer->addWidget(scroll);
}

int ChargerWnd::comboIndexForData(QComboBox* combo, int value)
{
    for(int i = 0; i < combo->count(); i++)
        if(combo->itemData(i).toInt() == value) return i;
    return -1;
}

bool ChargerWnd::readRegisterField(int reg, int mask, int shift, int* value)
{
    int regVal = 0;
    if(!m_device->getChargerRegister(reg, &regVal)) return false;
    *value = (regVal & mask) >> shift;
    return true;
}

bool ChargerWnd::setRegisterField(int reg, int mask, int shift, int value)
{
    int regVal = 0;
    if(!m_device->getChargerRegister(reg, &regVal)) return false;   /* read-modify-write */
    regVal = (regVal & ~mask) | ((value << shift) & mask);
    return m_device->setChargerRegister(reg, regVal);
}

void ChargerWnd::updateChargingModeLabel(const QString& modeCode)
{
    QString text;
    if(modeCode == "cc")        text = "Constant Current (CC)";
    else if(modeCode == "cv")   text = "Constant Voltage (CV)";
    else if(modeCode == "done") text = "Charge done";
    else if(modeCode == "not")  text = "Not charging";
    else                        text = modeCode;
    chargingModeLabel->setText(text);
}

void ChargerWnd::readStatus()
{
    int stat0 = 0;
    if(!m_device->getChargerRegister(BQ25180_REG_STAT0, &stat0))
    {
        stat0Label->setText("read error");
        return;
    }

    int chg = (stat0 >> 5) & 0x03;   /* CHG_STAT[1:0] */
    QString modeCode = (chg == 1) ? "cc" : (chg == 2) ? "cv" : (chg == 3) ? "done" : "not";
    updateChargingModeLabel(modeCode);

    QString s = QString("0x%1").arg(stat0, 2, 16, QChar('0')).toUpper();
    if(stat0 & 0x01) s += "  VIN:good"; else s += "  VIN:---";
    if(stat0 & 0x02) s += "  THERMREG";
    stat0Label->setText(s);
}

void ChargerWnd::onReadClicked()
{
    if(m_device == nullptr) return;

    QString s;
    if(m_device->getChargerHWSerial(&s))  hwSerialEdit->setText(s);
    QString v;
    if(m_device->getChargerFWVersion(&v)) fwVersionEdit->setText(v);

    float tv;
    if(m_device->getChargerTermVoltage(&tv)) termVoltageEdit->setText(QString::number(tv, 'f', 2));

    int tc;
    if(m_device->getChargerTermCurrent(&tc))
    {
        int idx = comboIndexForData(termCurrentCombo, tc);
        if(idx >= 0) termCurrentCombo->setCurrentIndex(idx);
    }

    int cc;
    if(m_device->getChargerCurrent(&cc)) chargeCurrentEdit->setText(QString::number(cc));

    int mc;
    if(m_device->getChargerMaxChargingCurrent(&mc))
    {
        int idx = comboIndexForData(maxCurrentCombo, mc);
        if(idx >= 0) maxCurrentCombo->setCurrentIndex(idx);
    }

    int buvlo;
    if(readRegisterField(BQ25180_REG_CHARGERCTRL1, BUVLO_MASK, BUVLO_SHIFT, &buvlo))
    {
        if(buvlo < 2) buvlo = 2;   /* codes 0/1/2 all map to 3.0 V */
        int idx = comboIndexForData(buvloCombo, buvlo);
        if(idx >= 0) buvloCombo->setCurrentIndex(idx);
    }

    int vlowv;
    if(readRegisterField(BQ25180_REG_IC_CTRL, VLOWV_MASK, VLOWV_SHIFT, &vlowv))
    {
        int idx = comboIndexForData(vlowvCombo, vlowv);
        if(idx >= 0) vlowvCombo->setCurrentIndex(idx);
    }

    readStatus();

    statusLabel->setText("Parameters read from device.");
}

void ChargerWnd::onSetClicked()
{
    if(m_device == nullptr) return;

    bool ok = true;
    ok &= m_device->setChargerTermVoltage(termVoltageEdit->text().toFloat());
    ok &= m_device->setChargerTermCurrent(termCurrentCombo->currentData().toInt());
    ok &= m_device->setChargerCurrent(chargeCurrentEdit->text().toInt());
    ok &= m_device->setChargerMaxChargingCurrent(maxCurrentCombo->currentData().toInt());
    ok &= setRegisterField(BQ25180_REG_CHARGERCTRL1, BUVLO_MASK, BUVLO_SHIFT, buvloCombo->currentData().toInt());
    ok &= setRegisterField(BQ25180_REG_IC_CTRL,      VLOWV_MASK, VLOWV_SHIFT, vlowvCombo->currentData().toInt());

    statusLabel->setText(ok ? "Parameters applied." : "Failed to apply one or more parameters.");

    /* Reflect what the device actually stored. */
    onReadClicked();
}

void ChargerWnd::onStoreClicked()
{
    if(m_device == nullptr) return;
    /* Persist the currently applied settings to EEPROM. "Set" should be used
     * first so the values the firmware stores are the intended ones. */
    bool ok = m_device->storeChargerConfig();
    statusLabel->setText(ok ? "Settings stored to EEPROM." : "Failed to store settings.");
}

void ChargerWnd::onApplyChemistryClicked()
{
    if(m_device == nullptr) return;

    float termV;
    int   vlowvCode;
    int   buvloCode;

    switch(chemistryCombo->currentIndex())
    {
        case 0:  /* Li-Ion / LiPo */
            termV = 4.20f; vlowvCode = 0 /*3.0V*/; buvloCode = 2 /*3.0V*/;
            break;
        case 1:  /* LiFePO4 */
            termV = 3.60f; vlowvCode = 1 /*2.8V*/; buvloCode = 5 /*2.4V*/;
            break;
        default:
            return;
    }

    /* Push preset into the editable fields, then apply via the normal path. */
    termVoltageEdit->setText(QString::number(termV, 'f', 2));
    int vIdx = comboIndexForData(vlowvCombo, vlowvCode); if(vIdx >= 0) vlowvCombo->setCurrentIndex(vIdx);
    int bIdx = comboIndexForData(buvloCombo, buvloCode); if(bIdx >= 0) buvloCombo->setCurrentIndex(bIdx);

    onSetClicked();
    statusLabel->setText("Chemistry preset applied: " + chemistryCombo->currentText());
}

void ChargerWnd::onChargerModeObtained(QString mode)
{
    updateChargingModeLabel(mode.trimmed().toLower());
}

void ChargerWnd::onStartChargingClicked()
{
    if(m_device == nullptr) return;
    bool ok = m_device->setChargerStatus(true);
    statusLabel->setText(ok ? "Charging started." : "Failed to start charging.");
}

void ChargerWnd::onStopChargingClicked()
{
    if(m_device == nullptr) return;
    bool ok = m_device->setChargerStatus(false);
    statusLabel->setText(ok ? "Charging stopped." : "Failed to stop charging.");
}

/* ---- EEPROM ---- */

void ChargerWnd::renderEeprom(const QByteArray& data)
{
    const int bytesPerLine = 16;
    QString formatted;

    for(int i = 0; i < data.size(); i++)
    {
        if((i % bytesPerLine) == 0)
            formatted += QString("%1: ").arg(i, 8, 16, QChar('0')).toUpper();

        quint8 byte = (quint8)data.at(i);
        QString hex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

        bool changed = (i < m_prevEepromData.size()) && (byte != (quint8)m_prevEepromData.at(i));
        if(changed) formatted += "<span style=\"color:red;font-weight:bold;\">" + hex + "</span> ";
        else        formatted += hex + " ";

        if((i % bytesPerLine) == bytesPerLine - 1 || i == data.size() - 1)
        {
            int lineStart = i - (i % bytesPerLine);
            QString ascii = " |";
            for(int j = lineStart; j <= i; j++)
            {
                quint8 c = (quint8)data.at(j);
                ascii += (c >= 32 && c <= 126) ? QChar(c) : QChar('.');
            }
            ascii += "|";
            formatted += ascii + "<br>";
        }
    }

    eepromEdit->setHtml("<pre style='font-family:Courier New;'>" + formatted + "</pre>");
    m_prevEepromData = data;
}

void ChargerWnd::onEepromReadClicked()
{
    if(m_device == nullptr) return;
    QByteArray data;
    if(!m_device->readChargerEeprom(&data))
    {
        statusLabel->setText("EEPROM read failed.");
        return;
    }
    m_eepromData = data;
    renderEeprom(m_eepromData);
    statusLabel->setText(QString("EEPROM read (%1 bytes).").arg(data.size()));
}

void ChargerWnd::onEepromWriteClicked()
{
    if(m_device == nullptr) return;

    if(m_eepromData.isEmpty())
    {
        statusLabel->setText("Nothing to write - Read or Import first.");
        return;
    }

    int size = m_device->getChargerEepromSize();
    if(size > 0 && m_eepromData.size() != size)
    {
        QMessageBox::warning(this, "EEPROM",
                             QString("EEPROM is %1 bytes, buffer holds %2.").arg(size).arg(m_eepromData.size()));
        return;
    }

    if(!m_device->writeChargerEeprom(m_eepromData))
    {
        statusLabel->setText("EEPROM write failed.");
        return;
    }
    onEepromReadClicked();   /* refresh view from device first */
    statusLabel->setText("EEPROM image successfully written.");   /* final message */
}

void ChargerWnd::onEepromFormatClicked()
{
    if(m_device == nullptr) return;
    if(QMessageBox::question(this, "Format EEPROM",
                             "Erase the entire EEPROM to 0xFF?") != QMessageBox::Yes) return;

    if(!m_device->formatChargerEeprom())
    {
        statusLabel->setText("EEPROM format failed.");
        return;
    }
    statusLabel->setText("EEPROM formatted.");
    onEepromReadClicked();
}

void ChargerWnd::onEepromImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import EEPROM image", "",
                                                    "Binary (*.bin);;All files (*)");
    if(fileName.isEmpty()) return;

    QFile f(fileName);
    if(!f.open(QIODevice::ReadOnly))
    {
        statusLabel->setText("Cannot open file for reading.");
        return;
    }
    m_eepromData = f.readAll();
    f.close();

    renderEeprom(m_eepromData);
    statusLabel->setText(QString("Imported %1 bytes (press Write to program).").arg(m_eepromData.size()));
}

void ChargerWnd::onEepromExportClicked()
{
    if(m_eepromData.isEmpty())
    {
        statusLabel->setText("Nothing to export - Read or Import first.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Export EEPROM image", "charger_eeprom.bin",
                                                    "Binary (*.bin);;All files (*)");
    if(fileName.isEmpty()) return;

    QFile f(fileName);
    if(!f.open(QIODevice::WriteOnly))
    {
        statusLabel->setText("Cannot open file for writing.");
        return;
    }
    f.write(m_eepromData);
    f.close();
    statusLabel->setText("EEPROM exported to " + fileName);
}

#include "aboutwnd.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

AboutWnd::AboutWnd(QWidget *parent) :
    QDialog(parent)
{
    QFont defaultFont("Arial", 10);
    setFont(defaultFont);

    setWindowTitle("About OpenEPT");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *mainVL = new QVBoxLayout(this);

    QHBoxLayout *headerHB = new QHBoxLayout();

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/images/main.png").scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setFixedSize(72, 72);

    QVBoxLayout *titleVL = new QVBoxLayout();

    QLabel *nameLabel = new QLabel("OpenEPT", this);
    QFont nameFont("Arial", 16);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    QLabel *subtitleLabel = new QLabel("MCU Energy Profiling Tool", this);

    QLabel *versionLabel = new QLabel(QString("Version %1").arg(APP_VERSION), this);
    versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    titleVL->addWidget(nameLabel);
    titleVL->addWidget(subtitleLabel);
    titleVL->addWidget(versionLabel);
    titleVL->addStretch();

    headerHB->addWidget(iconLabel);
    headerHB->addSpacing(15);
    headerHB->addLayout(titleVL);
    headerHB->addStretch();

    QLabel *descriptionLabel = new QLabel(
        "OpenEPT is an open source energy profiler for embedded systems. It controls the "
        "OpenEPT acquisition board, streams voltage and current samples in real time, and "
        "provides tools for energy consumption measurement, energy debugging and offline "
        "analysis of captured profiles.",
        this);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setMinimumWidth(430);
    descriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    QLabel *linksLabel = new QLabel(
        "<a href=\"https://www.openept.net\">www.openept.net</a>",
        this);
    linksLabel->setOpenExternalLinks(true);

    QLabel *creditsLabel = new QLabel(
        "Supported by <a href=\"https://nlnet.nl\">NLnet Foundation</a>.",
        this);
    creditsLabel->setOpenExternalLinks(true);

    QLabel *copyrightLabel = new QLabel("Copyright © OpenEPT Team", this);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setFixedSize(90, 30);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &AboutWnd::reject);

    mainVL->addLayout(headerHB);
    mainVL->addSpacing(10);
    mainVL->addWidget(descriptionLabel);
    mainVL->addSpacing(10);
    mainVL->addWidget(linksLabel);
    mainVL->addSpacing(10);
    mainVL->addWidget(creditsLabel);
    mainVL->addWidget(copyrightLabel);
    mainVL->addSpacing(10);
    mainVL->addWidget(buttonBox);

    setLayout(mainVL);
    mainVL->setSizeConstraint(QLayout::SetFixedSize);
}

#include "plotdockwidget.h"

#include <QHBoxLayout>
#include <QStyle>

#define TITLE_BAR_HEIGHT    22
#define TITLE_BUTTON_SIZE   18

PlotDockWidget::PlotDockWidget(QString aTitle, QWidget *parent)
    : QDockWidget(aTitle, parent)
{
    plotTitle = aTitle;
    maximized = false;

    setObjectName(aTitle + "PlotDock");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(QDockWidget::DockWidgetMovable |
                QDockWidget::DockWidgetFloatable |
                QDockWidget::DockWidgetClosable);

    titleBar = new QWidget(this);
    titleBar->setFixedHeight(TITLE_BAR_HEIGHT);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(6, 0, 2, 0);
    titleLayout->setSpacing(2);

    titleLabel = new QLabel(titleBar);
    titleLabel->setStyleSheet("font-weight: bold;");

    maximizeButton = createTitleButton("Maximize");
    maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    floatButton = createTitleButton("Float");
    floatButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    closeButton = createTitleButton("Close");
    closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(maximizeButton);
    titleLayout->addWidget(floatButton);
    titleLayout->addWidget(closeButton);

    setTitleBarWidget(titleBar);

    connect(maximizeButton, SIGNAL(clicked()), this, SLOT(onMaximizeClicked()));
    connect(floatButton, SIGNAL(clicked()), this, SLOT(onFloatClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
    connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(onTopLevelChanged(bool)));

    updateTitleLabel();
}

QToolButton* PlotDockWidget::createTitleButton(QString aToolTip)
{
    QToolButton *button = new QToolButton(titleBar);
    button->setAutoRaise(true);
    button->setFixedSize(TITLE_BUTTON_SIZE, TITLE_BUTTON_SIZE);
    button->setToolTip(aToolTip);
    return button;
}

void PlotDockWidget::setPlotTitle(QString aTitle)
{
    plotTitle = aTitle;
    updateTitleLabel();
}

void PlotDockWidget::setDeviceName(QString aDeviceName)
{
    deviceName = aDeviceName;
    updateTitleLabel();
}

void PlotDockWidget::setMaximized(bool aMaximized)
{
    maximized = aMaximized;
    if(maximized)
    {
        maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
        maximizeButton->setToolTip("Restore");
    }
    else
    {
        maximizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
        maximizeButton->setToolTip("Maximize");
    }
}

bool PlotDockWidget::isMaximized()
{
    return maximized;
}

void PlotDockWidget::updateTitleLabel()
{
    QString title = plotTitle;
    if(isFloating() && !deviceName.isEmpty())
    {
        title += " - " + deviceName;
    }
    titleLabel->setText(title);
    setWindowTitle(title);
}

void PlotDockWidget::onMaximizeClicked()
{
    setMaximized(!maximized);
    emit sigMaximizeToggled(this, maximized);
}

void PlotDockWidget::onFloatClicked()
{
    setFloating(!isFloating());
}

void PlotDockWidget::onCloseClicked()
{
    close();
}

void PlotDockWidget::onTopLevelChanged(bool floating)
{
    if(floating)
    {
        floatButton->setToolTip("Dock");
        floatButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton));
        maximizeButton->setEnabled(false);
    }
    else
    {
        floatButton->setToolTip("Float");
        floatButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
        maximizeButton->setEnabled(true);
    }
    updateTitleLabel();
}

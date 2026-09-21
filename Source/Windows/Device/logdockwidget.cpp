#include "logdockwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QStyle>

LogDockWidget::LogDockWidget(QString aTitle, QWidget *parent)
    : QDockWidget(aTitle, parent)
{
    static const char* filterNames[4] = {"All messages", "DUT info only", "DUT info + energy points", "Energy points only"};
    QWidget *content = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(content);
    QHBoxLayout *toolRow = new QHBoxLayout();

    filter = LOG_FILTER_ALL;
    setObjectName("logDock");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    setAllowedAreas(Qt::AllDockWidgetAreas);

    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    toolRow->setContentsMargins(0, 0, 0, 0);
    toolRow->setSpacing(4);

    filterButton = new QToolButton(content);
    filterButton->setIcon(QIcon(":/images/NewSet/filter.png"));
    filterButton->setIconSize(QSize(18, 18));
    filterButton->setFixedSize(24, 24);
    filterButton->setToolTip("Filter: " + QString(filterNames[0]));
    filterButton->setPopupMode(QToolButton::InstantPopup);
    filterButton->setAutoRaise(true);
    filterButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    filterMenu = new QMenu(filterButton);
    filterMenu->setFont(QFont("Arial", 10));
    filterGroup = new QActionGroup(filterMenu);
    filterGroup->setExclusive(true);
    for(int i = 0; i < 4; i++)
    {
        QAction *action = filterMenu->addAction(filterNames[i]);
        action->setCheckable(true);
        action->setData(i);
        action->setChecked(i == 0);
        filterGroup->addAction(action);
    }
    filterButton->setMenu(filterMenu);

    floatButton = new QToolButton(content);
    floatButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    floatButton->setFixedSize(24, 24);
    floatButton->setToolTip("Float");
    floatButton->setAutoRaise(true);

    clearButton = new QToolButton(content);
    clearButton->setIcon(QIcon(":/images/NewSet/format.png"));
    clearButton->setIconSize(QSize(18, 18));
    clearButton->setFixedSize(24, 24);
    clearButton->setToolTip("Clear log");
    clearButton->setAutoRaise(true);

    toolRow->addStretch();
    toolRow->addWidget(filterButton);
    toolRow->addWidget(clearButton);
    toolRow->addWidget(floatButton);

    textEdit = new QPlainTextEdit(content);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Arial", 10));
    textEdit->setMinimumHeight(60);

    layout->addLayout(toolRow);
    layout->addWidget(textEdit, 1);
    setWidget(content);
    setTitleBarWidget(new QWidget(this));

    connect(filterGroup, SIGNAL(triggered(QAction*)), this, SLOT(onFilterActionTriggered(QAction*)));
    connect(clearButton, SIGNAL(clicked()), this, SIGNAL(sigClearRequested()));
    connect(floatButton, SIGNAL(clicked()), this, SLOT(onFloatClicked()));
    connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(onTopLevelChanged(bool)));
}

QPlainTextEdit* LogDockWidget::getTextWidget()
{
    return textEdit;
}

log_filter_t LogDockWidget::getFilter()
{
    return filter;
}

void LogDockWidget::setFilter(log_filter_t aFilter)
{
    QList<QAction*> actions = filterGroup->actions();
    for(int i = 0; i < actions.size(); i++)
    {
        if(actions[i]->data().toInt() == (int)aFilter)
        {
            actions[i]->setChecked(true);
            onFilterActionTriggered(actions[i]);
            break;
        }
    }
}

void LogDockWidget::onFilterActionTriggered(QAction* action)
{
    if(action == NULL) return;
    filter = (log_filter_t)action->data().toInt();
    filterButton->setToolTip("Filter: " + action->text());
    emit sigFilterChanged((int)filter);
}

void LogDockWidget::onFloatClicked()
{
    setFloating(!isFloating());
}

void LogDockWidget::onTopLevelChanged(bool floating)
{
    if(floating)
    {
        setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
        setWindowTitle("Log");
        setMinimumSize(400, 200);
        floatButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarShadeButton));
        floatButton->setToolTip("Dock");
        show();
    }
    else
    {
        setMinimumSize(0, 0);
        floatButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
        floatButton->setToolTip("Float");
    }
}

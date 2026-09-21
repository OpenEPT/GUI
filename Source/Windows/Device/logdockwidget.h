#ifndef LOGDOCKWIDGET_H
#define LOGDOCKWIDGET_H

#include <QDockWidget>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QLabel>
#include <QMenu>
#include <QActionGroup>
#include "Utility/log.h"

class LogDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit            LogDockWidget(QString aTitle, QWidget *parent = nullptr);

    QPlainTextEdit*     getTextWidget();
    log_filter_t        getFilter();
    void                setFilter(log_filter_t aFilter);

signals:
    void                sigFilterChanged(int filter);
    void                sigClearRequested();

private slots:
    void                onFilterActionTriggered(QAction* action);
    void                onFloatClicked();
    void                onTopLevelChanged(bool floating);

private:
    QPlainTextEdit      *textEdit;
    QToolButton         *filterButton;
    QMenu               *filterMenu;
    QActionGroup        *filterGroup;
    QToolButton         *clearButton;
    QToolButton         *floatButton;
    log_filter_t        filter;
};

#endif

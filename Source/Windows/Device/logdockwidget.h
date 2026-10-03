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
    bool                getFollowOutput();
    void                setFollowOutput(bool aFollow);

signals:
    void                sigFilterChanged(int filter);
    void                sigClearRequested();
    void                sigFollowOutputChanged(bool follow);

private slots:
    void                onFilterActionTriggered(QAction* action);
    void                onFollowButtonToggled(bool checked);
    void                onFloatClicked();
    void                onTopLevelChanged(bool floating);

private:
    QPlainTextEdit      *textEdit;
    QToolButton         *filterButton;
    QMenu               *filterMenu;
    QActionGroup        *filterGroup;
    QToolButton         *followButton;
    QToolButton         *clearButton;
    QToolButton         *floatButton;
    log_filter_t        filter;
};

#endif

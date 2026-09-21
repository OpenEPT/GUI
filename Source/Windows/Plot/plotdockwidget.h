#ifndef PLOTDOCKWIDGET_H
#define PLOTDOCKWIDGET_H

#include <QDockWidget>
#include <QLabel>
#include <QToolButton>
#include <QString>

class PlotDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit            PlotDockWidget(QString aTitle, QWidget *parent = nullptr);

    void                setPlotTitle(QString aTitle);
    void                setDeviceName(QString aDeviceName);
    void                setMaximized(bool aMaximized);
    bool                isMaximized();

signals:
    void                sigMaximizeToggled(PlotDockWidget* dock, bool maximized);

private slots:
    void                onMaximizeClicked();
    void                onFloatClicked();
    void                onCloseClicked();
    void                onTopLevelChanged(bool floating);

private:
    QToolButton*        createTitleButton(QString aToolTip);
    void                updateTitleLabel();

    QString             plotTitle;
    QString             deviceName;
    bool                maximized;

    QWidget             *titleBar;
    QLabel              *titleLabel;
    QToolButton         *maximizeButton;
    QToolButton         *floatButton;
    QToolButton         *closeButton;
};

#endif

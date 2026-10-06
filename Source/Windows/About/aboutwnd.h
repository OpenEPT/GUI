#ifndef ABOUTWND_H
#define ABOUTWND_H

#include <QDialog>

#ifndef APP_VERSION
#define APP_VERSION "0.0.0-dev"
#endif

class UpdateChecker;

class AboutWnd : public QDialog
{
    Q_OBJECT

public:
    explicit AboutWnd(QWidget *parent = nullptr);

private:
    UpdateChecker *updateChecker;
};

#endif // ABOUTWND_H

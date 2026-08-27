#include "openept.h"
#include "Processing/dataprocessing.h"
#include "Processing/charginganalysis.h"
#include "Windows/WSSelection/selectworkspace.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
    QToolTip {
        background-color: #ffffff;
        color: #000000;
        border: 1px solid #ccc;
    }
    )");

    QCoreApplication::setOrganizationName("OpenEPT");
    QCoreApplication::setOrganizationDomain("openept.net");
    QCoreApplication::setApplicationName("OpenEPT");

    SelectWorkspace wsSelection;

    if(wsSelection.exec() == QDialog::Accepted)
    {
        OpenEPT w(wsSelection.getWSPath());
        w.setWindowState(Qt::WindowMaximized);
        w.show();

        qRegisterMetaType<dataprocessing_consumption_mode_t>("dataprocessing_consumption_mode_t");
        qRegisterMetaType<dataprocessing_dev_info_t>("dataprocessing_dev_info_t");
        qRegisterMetaType<charginganalysis_status_t>("charginganalysis_status_t");

        return a.exec();
    }

    return -1;
}

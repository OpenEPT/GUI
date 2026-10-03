QT       += core gui opengl concurrent
QT       += network
QT       += svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17

isEmpty(APP_VERSION) {
    APP_VERSION = $$cat($$PWD/../VERSION, lines)
}
isEmpty(APP_VERSION) {
    APP_VERSION = 0.0.0-dev
}
VERSION = $$replace(APP_VERSION, "[-+].*", "")
DEFINES += APP_VERSION=\\\"$$APP_VERSION\\\"
#DEFINES += QCUSTOMPLOT_USE_OPENGL
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
unix:!macx {
    LIBS     += $$PWD/../libs/linux/libfftw3.a
}
win32 {
    LIBS     += "../libs/win/libfftw3-3.lib"
    LIBS     += -lws2_32
}

SOURCES += \
    Chart/qcustomplot.cpp \
    Links/controllink.cpp \
    Links/edlink.cpp \
    Links/statuslink.cpp \
    Links/streamlink.cpp \
    Processing/Parameters/applicationparamdefs.cpp \
    Processing/Parameters/applicationparameters.cpp \
    Processing/Parameters/deviceparamdefs.cpp \
    Processing/Parameters/deviceparameters.cpp \
    Processing/Parameters/parameterstore.cpp \
    Processing/batteryparamsextraction.cpp \
    Processing/calibrationdata.cpp \
    Processing/charginganalysis.cpp \
    Processing/dataprocessing.cpp \
    Processing/epprocessing.cpp \
    Processing/fileprocessing.cpp \
    Processing/waveform.cpp \
    Utility/log.cpp \
    Windows/About/aboutwnd.cpp \
    Windows/BatteryParams/batterycyclewnd.cpp \
    Windows/BatteryParams/batteryfitintervalwnd.cpp \
    Windows/BatteryParams/batteryfitqualitywnd.cpp \
    Windows/BatteryParams/batteryparamsbarwnd.cpp \
    Windows/BatteryParams/batteryparamsplotsettings.cpp \
    Windows/BatteryParams/batteryocvanalysiswnd.cpp \
    Windows/BatteryParams/batteryparamssettingsdlg.cpp \
    Windows/BatteryParams/batteryparamstrendwnd.cpp \
    Windows/BatteryParams/batteryparamswnd.cpp \
    Windows/AddDevice/adddevicewnd.cpp \
    Windows/ApplicationConf/applicationconfwnd.cpp \
    Windows/Console/consolewnd.cpp \
    Windows/DataAnalyzer/dataanalyzer.cpp \
    Windows/DataAnalyzer/dataanalyzerprofile.cpp \
    Windows/DataAnalyzer/dataanalyzerstatistics.cpp \
    Windows/Device/calibrationwnd.cpp \
    Windows/Device/autocalibrationwnd.cpp \
    Windows/Device/configurationwnd.cpp \
    Windows/Device/datastatistics.cpp \
    Windows/Device/devicewnd.cpp \
    Windows/Device/energycontrolwnd.cpp \
    Windows/Plot/plot.cpp \
    Windows/Plot/plotdockwidget.cpp \
    Windows/Device/logdockwidget.cpp \
    Windows/WSSelection/selectworkspace.cpp \
    device.cpp \
    devicecontainer.cpp \
    main.cpp \
    openept.cpp

HEADERS += \
    Chart/qcustomplot.h \
    Links/controllink.h \
    Links/edlink.h \
    Links/statuslink.h \
    Links/streamlink.h \
    Processing/Parameters/applicationparamdefs.h \
    Processing/Parameters/applicationparameters.h \
    Processing/Parameters/deviceparamdefs.h \
    Processing/Parameters/deviceparameters.h \
    Processing/Parameters/parameterdefs.h \
    Processing/Parameters/parameterstore.h \
    Processing/batteryparamsextraction.h \
    Processing/calibrationdata.h \
    Processing/charginganalysis.h \
    Processing/dataprocessing.h \
    Processing/epprocessing.h \
    Processing/fftw/fftw3.h \
    Processing/fileprocessing.h \
    Processing/waveform.h \
    Utility/log.h \
    Windows/About/aboutwnd.h \
    Windows/BatteryParams/batterycyclewnd.h \
    Windows/BatteryParams/batteryfitintervalwnd.h \
    Windows/BatteryParams/batteryfitqualitywnd.h \
    Windows/BatteryParams/batteryparamsbarwnd.h \
    Windows/BatteryParams/batteryparamsplotsettings.h \
    Windows/BatteryParams/batteryocvanalysiswnd.h \
    Windows/BatteryParams/batteryparamssettingsdlg.h \
    Windows/BatteryParams/batteryparamstrendwnd.h \
    Windows/BatteryParams/batteryparamswnd.h \
    Windows/AddDevice/adddevicewnd.h \
    Windows/ApplicationConf/applicationconfwnd.h \
    Windows/Console/consolewnd.h \
    Windows/DataAnalyzer/dataanalyzer.h \
    Windows/DataAnalyzer/dataanalyzerprofile.h \
    Windows/DataAnalyzer/dataanalyzerworker.h \
    Windows/DataAnalyzer/dataanalyzerstatistics.h \
    Windows/Device/calibrationwnd.h \
    Windows/Device/autocalibrationwnd.h \
    Windows/Device/configurationwnd.h \
    Windows/Device/datastatistics.h \
    Windows/Device/devicewnd.h \
    Windows/Device/energycontrolwnd.h \
    Windows/Plot/plot.h \
    Windows/Plot/plotdockwidget.h \
    Windows/Device/logdockwidget.h \
    Windows/WSSelection/selectworkspace.h \
    device.h \
    devicecontainer.h \
    openept.h

FORMS += \
    Windows/AddDevice/adddevicewnd.ui \
    Windows/ApplicationConf/applicationconfwnd.ui \
    Windows/Console/consolewnd.ui \
    Windows/DataAnalyzer/dataanalyzer.ui \
    Windows/Device/calibrationwnd.ui \
    Windows/Device/configurationwnd.ui \
    Windows/Device/datastatistics.ui \
    Windows/Device/devicewnd.ui \
    Windows/Device/energycontrolwnd.ui \
    Windows/WSSelection/selectworkspace.ui \
    openept.ui

RC_ICONS = main.ico

QTPLUGIN += qjpeg
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

win32 {
    DLL_SRC = $$absolute_path(../libs/win/libfftw3-3.dll)
    DLL_SRC = $$replace(DLL_SRC, /, \\)

    DLL_BASE_DST = $$OUT_PWD

    CONFIG(debug, debug|release) {
        DLL_DST_DIR = $$DLL_BASE_DST\\debug
    } else {
        DLL_DST_DIR = $$DLL_BASE_DST\\release
    }

    QMAKE_POST_LINK += powershell -Command \"Copy-Item -Path '$$DLL_SRC' -Destination '$$DLL_DST_DIR'\"

    message("Post-build copy: $$DLL_SRC -> $$DLL_DST_DIR")
}

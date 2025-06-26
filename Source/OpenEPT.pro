QT       += core gui opengl concurrent
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17
#DEFINES += QCUSTOMPLOT_USE_OPENGL
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
unix:!macx {
    LIBS     += "/home/elektronika/Desktop/Projects/OpenEPT/Forks/OpenEPT_GUI/Source/libs/linux/libfftw3.a"
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
    Processing/calibrationdata.cpp \
    Processing/charginganalysis.cpp \
    Processing/dataprocessing.cpp \
    Processing/epprocessing.cpp \
    Processing/fileprocessing.cpp \
    Utility/log.cpp \
    Windows/AddDevice/adddevicewnd.cpp \
    Windows/Console/consolewnd.cpp \
    Windows/DataAnalyzer/dataanalyzer.cpp \
    Windows/Device/advanceconfigurationwnd.cpp \
    Windows/Device/calibrationwnd.cpp \
    Windows/Device/datastatistics.cpp \
    Windows/Device/devicewnd.cpp \
    Windows/Device/energycontrolwnd.cpp \
    Windows/Plot/plot.cpp \
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
    Processing/calibrationdata.h \
    Processing/charginganalysis.h \
    Processing/dataprocessing.h \
    Processing/epprocessing.h \
    Processing/fftw/fftw3.h \
    Processing/fileprocessing.h \
    Utility/log.h \
    Windows/AddDevice/adddevicewnd.h \
    Windows/Console/consolewnd.h \
    Windows/DataAnalyzer/dataanalyzer.h \
    Windows/Device/advanceconfigurationwnd.h \
    Windows/Device/calibrationwnd.h \
    Windows/Device/datastatistics.h \
    Windows/Device/devicewnd.h \
    Windows/Device/energycontrolwnd.h \
    Windows/Plot/plot.h \
    Windows/WSSelection/selectworkspace.h \
    device.h \
    devicecontainer.h \
    openept.h

FORMS += \
    Windows/AddDevice/adddevicewnd.ui \
    Windows/Console/consolewnd.ui \
    Windows/DataAnalyzer/dataanalyzer.ui \
    Windows/Device/advanceconfigurationwnd.ui \
    Windows/Device/calibrationwnd.ui \
    Windows/Device/datastatistics.ui \
    Windows/Device/devicewnd.ui \
    Windows/Device/energycontrolwnd.ui \
    Windows/WSSelection/selectworkspace.ui \
    openept.ui

QTPLUGIN += qjpeg
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

#win32 {

#    message("Copy .dll file to output dir...")

#    DLL_SRC = ../libs/win/libfftw3-3.dll
#    ABS_DLL_SRC = $$absolute_path($$DLL_SRC)
#    DLL_DST = $$OUT_PWD

#    message("Build output directory: $$DLL_DST")
#    message("Project directory: $$PWD")

#    !exists($$ABS_DLL_SRC): message("ERROR: DLL file not found at path: $$ABS_DLL_SRC")
#    else: message("DLL file found: $$ABS_DLL_SRC")

#    QMAKE_POST_LINK += $$QMAKE_COPY  $$quote($$ABS_DLL_SRC) $$quote($$DLL_DST) $$escape_expand(\\n\\t)

#    message("Post build command: $$QMAKE_POST_LINK")
#    export(QMAKE_POST_LINK)
#}

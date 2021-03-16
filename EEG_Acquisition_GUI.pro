QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH +=E:\Anaconda3\include   ############# python enviroment
LIBS += -LE:\Anaconda3\libs\
-l_tkinter\
-lpython3\
-lpython37

SOURCES += \
    acquisitionwindow.cpp \
    analysistools.cpp \
    charthelp.cpp \
    dwtinfo.cpp \
    edflib.c \
    filter.cpp \
    filtersetting.cpp \
    main.cpp \
    mainbackground.cpp \
    mainwindow.cpp \
    p300.cpp \
    p300oddballsetting.cpp \
    preprocesswindow.cpp \
    psd.cpp \
    psdinfo.cpp \
    setchannelname.cpp \
    seteventchannel.cpp \
    setinfo.cpp \
    wigner.cpp \
    wignerinfo.cpp \
    workthread.cpp

HEADERS += \
    acquisitionwindow.h \
    analysistools.h \
    charthelp.h \
    dwtinfo.h \
    edflib.h \
    filter.h \
    filtersetting.h \
    mainbackground.h \
    mainwindow.h \
    p300.h \
    p300oddballsetting.h \
    preprocesswindow.h \
    psd.h \
    psdinfo.h \
    setchannelname.h \
    seteventchannel.h \
    setinfo.h \
    wigner.h \
    wignerinfo.h \
    workthread.h

FORMS += \
    acquisitionwindow.ui \
    charthelp.ui \
    dwtinfo.ui \
    filtersetting.ui \
    mainbackground.ui \
    mainwindow.ui \
    p300.ui \
    p300oddballsetting.ui \
    preprocesswindow.ui \
    psd.ui \
    psdinfo.ui \
    setchannelname.ui \
    seteventchannel.ui \
    setinfo.ui \
    wigner.ui \
    wignerinfo.ui

TRANSLATIONS += \
    EEG_Acquisition_GUI_zh_CN.ts

DISTFILES += \
    dataformatload.py \
    tf.py

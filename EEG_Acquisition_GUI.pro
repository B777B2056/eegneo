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

#INCLUDEPATH += E:\MATLAB\extern\include\
#LIBS += -llibeng -LE:\MATLAB\extern\lib\win64\microsoft
#LIBS += -llibmx -LE:\MATLAB\extern\lib\win64\microsoft
#LIBS += -llibmat -LE:\MATLAB\extern\lib\win64\microsoft

SOURCES += \
    acquisitionwindow.cpp \
    charthelp.cpp \
    edflib.c \
    filter.cpp \
    main.cpp \
    mainbackground.cpp \
    mainwindow.cpp \
    p300.cpp \
    p300oddballsetting.cpp \
    preprocesswindow.cpp \
    setchannelname.cpp \
    setinfo.cpp \
    workthread.cpp

HEADERS += \
    acquisitionwindow.h \
    charthelp.h \
    edflib.h \
    filter.h \
    mainbackground.h \
    mainwindow.h \
    p300.h \
    p300oddballsetting.h \
    preprocesswindow.h \
    setchannelname.h \
    setinfo.h \
    workthread.h

FORMS += \
    acquisitionwindow.ui \
    charthelp.ui \
    mainbackground.ui \
    mainwindow.ui \
    p300.ui \
    p300oddballsetting.ui \
    preprocesswindow.ui \
    setchannelname.ui \
    setinfo.ui

TRANSLATIONS += \
    EEG_Acquisition_GUI_zh_CN.ts

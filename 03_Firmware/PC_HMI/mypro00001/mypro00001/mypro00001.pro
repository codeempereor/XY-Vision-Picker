QT += core gui serialbus
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    dotmatrixview.cpp

HEADERS += \
    mainwindow.h \
    protocol.h \
    dotmatrixview.h

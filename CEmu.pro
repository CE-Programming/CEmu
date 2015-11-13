#-------------------------------------------------
#
# Project created by QtCreator 2015-10-26T17:20:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CEmu
TEMPLATE = app

QMAKE_CXXFLAGS += -Wall

SOURCES += main.cpp\
    mainwindow.cpp \
    cemusettings.cpp \
    optionswindow.cpp \
    romselection.cpp \
    aboutwindow.cpp \
    debugwindow.cpp \
    qtframebuffer.cpp \
    lcdwidget.cpp \
    core/asic.c \
    core/backlightport.c \
    core/context.c \
    core/controlport.c \
    core/cpu.c \
    core/flashport.c \
    core/keypad.c \
    core/lcd.c \
    core/memory.c \
    core/registers.c \
    core/runloop.c \
    core/apb.c \
    core/exxx.c \
    core/dxxx.c \
    core/cxxx.c \
    core/fxxx.c

HEADERS  += mainwindow.h \
    cemusettings.h \
    optionswindow.h \
    romselection.h \
    aboutwindow.h \
    debugwindow.h \
    qtframebuffer.h \
    lcdwidget.h \
    core/asic.h \
    core/backlightport.h \
    core/context.h \
    core/controlport.h \
    core/cpu.h \
    core/defines.h \
    core/flashport.h \
    core/keypad.h \
    core/lcd.h \
    core/memory.h \
    core/registers.h \
    core/runloop.h \
    core/tidevices.h \
    core/apb.h \
    core/exxx.h \
    core/dxxx.h \
    core/cxxx.h \
    core/fxxx.h

FORMS    += mainwindow.ui \
    optionswindow.ui \
    romselection.ui \
    aboutwindow.ui \
    debugwindow.ui

RESOURCES +=

RC_ICONS += icon.ico

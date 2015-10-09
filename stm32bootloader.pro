#-------------------------------------------------
#
# Project created by QtCreator 2015-09-30T11:03:35
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = stm32bootloader
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    bootloader.cpp

HEADERS  += mainwindow.h \
    bootloader.h

FORMS    += mainwindow.ui

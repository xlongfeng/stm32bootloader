#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "bootloader.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort(0),
    bootloader(0)
{
    ui->setupUi(this);
    QListIterator<QSerialPortInfo> portinfos(QSerialPortInfo::availablePorts());
    while (portinfos.hasNext()) {
        QSerialPortInfo portinfo = portinfos.next();
        qDebug() << portinfo.description() << portinfo.manufacturer() << portinfo.portName() << portinfo.serialNumber() << portinfo.systemLocation();
    }
    qDebug() << QSerialPortInfo::standardBaudRates();

    bootloader = new Bootloader(this);
    bootloader->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * STM32 Bootloader
 *
 * Copyright (c) 2015, longfeng.xiao <xlongfeng@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "settings.h"
#include "bootloader.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort(new QSerialPort),
    timer(new QTimer),
    bootloader(new Bootloader)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    connect(ui->portComboBox, SIGNAL(activated(QString)), this, SLOT(portChanged(QString)));
    connect(ui->baudrateComboBox, SIGNAL(activated(QString)), this, SLOT(baudrateChanged(QString)));
    connect(ui->resetPushButton, SIGNAL(pressed()), this, SLOT(resetEnterAction()));
    connect(ui->resetPushButton, SIGNAL(released()), this, SLOT(resetExitAction()));
    connect(ui->openPushButton, SIGNAL(pressed()), this, SLOT(openAction()));
    connect(ui->loadPushButton, SIGNAL(pressed()), this, SLOT(loadAction()));

    QListIterator<QSerialPortInfo> portinfos(QSerialPortInfo::availablePorts());
    while (portinfos.hasNext()) {
        QSerialPortInfo portinfo = portinfos.next();
        qDebug() << portinfo.description() << portinfo.manufacturer() << portinfo.portName() << portinfo.serialNumber() << portinfo.systemLocation();
        ui->portComboBox->addItem(portinfo.portName());
    }

    const QString &port = Settings::instance()->value("Port").toString();
    if (!port.isEmpty()) {
        int index = ui->portComboBox->findText(port);
        if (index > 0)
            ui->portComboBox->setCurrentIndex(index);
    }

    QListIterator<qint32> standardBaudRates(QSerialPortInfo::standardBaudRates());
    while (standardBaudRates.hasNext()) {
        qint32 baudrate = standardBaudRates.next();
        ui->baudrateComboBox->addItem(QString::number(baudrate));
    }

    quint32 baudrate = Settings::instance()->value("Baudrate", 115200).toUInt();
    if (1) {
        int index = ui->baudrateComboBox->findText(QString::number(baudrate));
        if (index > 0)
            ui->baudrateComboBox->setCurrentIndex(index);
    }

    const QString &filename = Settings::instance()->value("Filename").toString();
    if (!filename.isEmpty()) {
        ui->binLineEdit->setText(filename);
    }

    connect(timer, SIGNAL(timeout()), this, SLOT(readSerial()));
    connect(bootloader, SIGNAL(started()), this, SLOT(loadEnter()));
    connect(bootloader, SIGNAL(finished()), this, SLOT(loadExit()));
    connect(bootloader, SIGNAL(progressValue(int)), this, SLOT(loadProgress(int)));

    openSerial(portName(), baudrate);
}

MainWindow::~MainWindow()
{
    delete bootloader;
    delete timer;
    delete serialPort;
    delete ui;
}

void MainWindow::portChanged(const QString &text)
{
    openSerial(text, baudrate());
    Settings::instance()->setValue("Port", text);
}

void MainWindow::baudrateChanged(const QString &text)
{
    openSerial(portName(), text.toUInt());
    Settings::instance()->setValue("Baudrate", text);
}

void MainWindow::readSerial()
{
    if (serialPort->isOpen() && serialPort->bytesAvailable()) {
        ui->plainTextEdit->moveCursor(QTextCursor::End);
        ui->plainTextEdit->insertPlainText(serialPort->readAll());
        ui->plainTextEdit->moveCursor(QTextCursor::End);
    }
}

void MainWindow::resetEnterAction()
{
    if (serialPort->isOpen()) {
        serialPort->setDataTerminalReady(false);
        serialPort->setRequestToSend(true);
    }
}

void MainWindow::resetExitAction()
{
    if (serialPort->isOpen()) {
        serialPort->setDataTerminalReady(false);
        serialPort->setRequestToSend(false);
    }
}

void MainWindow::openAction()
{
    const QString &filename = QFileDialog::getOpenFileName(this, "", "", "Bin Format (*.bin)");
    if (filename.size() != 0) {
        Settings::instance()->setValue("Filename", filename);
        ui->binLineEdit->setText(filename);
    }
}

void MainWindow::loadAction()
{
    closeSerial();
    ui->openPushButton->setDisabled(true);
    bootloader->setPortName(portName());
    bootloader->setBaudrate(baudrate());
    bootloader->setFilename(filename());
    bootloader->start();
}

void MainWindow::loadEnter()
{
    int minimum, maximum;
    bootloader->progressRange(minimum, maximum);
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(minimum, maximum);
    ui->progressBar->setValue(minimum);
}

void MainWindow::loadExit()
{
    ui->progressBar->setVisible(false);
    ui->openPushButton->setEnabled(true);
    openSerial(portName(), baudrate());
    resetEnterAction();
    QTimer::singleShot(50, this, SLOT(resetExitAction()));
}

void MainWindow::loadProgress(int value)
{
    ui->progressBar->setValue(value);
}

QString MainWindow::portName() const
{
    return ui->portComboBox->currentText();
}

qint32 MainWindow::baudrate() const
{
    return ui->baudrateComboBox->currentText().toUInt();
}

QString MainWindow::filename() const
{
    return ui->binLineEdit->text();
}

void MainWindow::openSerial(const QString &port, qint32 baudrate)
{
    closeSerial();

    serialPort->setPortName(port);
    serialPort->setBaudRate(baudrate);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    if (!serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "serialPort open:" << serialPort->error();
        return;
    }

    serialPort->setDataTerminalReady(false);
    serialPort->setRequestToSend(false);

    timer->start(1000);

    qDebug() << "serialPort open:" << port << baudrate;
}

void MainWindow::closeSerial()
{
    timer->stop();

    if (serialPort->isOpen()) {
        serialPort->close();
    }
}

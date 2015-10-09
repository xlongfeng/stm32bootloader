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

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QThread>
#include <QMap>

class QSerialPort;

class Bootloader : public QThread
{
    Q_OBJECT

public:
    explicit Bootloader(QObject *parent = 0);
    ~Bootloader();

    void setPortName(const QString &portName)
    {
        this->portName = portName;
    }

    void setBaudrate(qint32 baudrate)
    {
        this->baudrate = baudrate;
    }

    void setFilename(const QString &filename)
    {
        this->filename = filename;
    }

    void progressRange(int &minimum, int &maximum)
    {
        minimum = 0;
        maximum = 100;
    }

Q_SIGNALS:
    void progressValue(int value);

protected:
    virtual void run();

private:
    bool openSerial();
    void closeSerial();
    void bootModeEnter();
    void bootModeExit();
    char checkSum(const QByteArray &data);
    qint64 write(char ch);
    qint64 writeCmd(char ch);
    qint64 writeAddr(quint32 addr);
    qint64 writeBytesRead(char size);
    qint64 writeData(const QByteArray &data);
    bool waitForRead(int msec);
    bool waitForAck(int msec = 10);
    bool autoBaudrateSeq();

private:
    QString portName;
    qint32 baudrate;
    QString filename;
    QSerialPort *serialPort;
    QByteArray buffer;
    static QMap<int, int> densityMap;
};

#endif // BOOTLOADER_H

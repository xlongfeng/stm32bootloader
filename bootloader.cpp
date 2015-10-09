#include <QDebug>
#include <QTime>
#include <QFile>
#include <QSerialPort>
#include "bootloader.h"

const char Ack = 0x79;
const char Nack = 0x1f;
const char GetCommand = 0x00;
const char GetVersionCommand = 0x01;
const char GetIDCommand = 0x02;
const char ReadMemoryCommand = 0x11;
const char WriteMemoryCommand = 0x31;
const char EraseMemoryCommand = 0x43;
const char ExtendedEraseMemoryCommand = 0x44;
const qint32 FlashBaseAddress = 0x08000000;

QMap<int, int> Bootloader::densityMap;

Bootloader::Bootloader(QObject *parent) :
    QThread(parent),
    serialPort(0)
{
    if (densityMap.empty()) {
        densityMap[0x412] = 1024;
        densityMap[0x410] = 1024;
        densityMap[0x414] = 2048;
        densityMap[0x418] = 2048;
        densityMap[0x420] = 1024;
        densityMap[0x428] = 2048;
        densityMap[0x430] = 2048;
        densityMap[0x436] = 256;
        densityMap[0x416] = 256;
    }
}

Bootloader::~Bootloader()
{

}

bool Bootloader::openSerial()
{
    serialPort = new QSerialPort();
    serialPort->setPortName(QLatin1String("COM8"));
    serialPort->setBaudRate(QSerialPort::Baud57600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setParity(QSerialPort::EvenParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    if (!serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "serialPort open:" << serialPort->error();
        return false;
    }

    return true;
}

void Bootloader::closeSerial()
{

}

void Bootloader::bootModeEnter()
{
    serialPort->flush();
    serialPort->setDataTerminalReady(true);
    serialPort->setRequestToSend(true);
    msleep(100);
    serialPort->readAll();
    serialPort->setRequestToSend(false);
    msleep(10);
}

void Bootloader::bootModeExit()
{
    if (serialPort) {
        if (serialPort->isOpen()) {
            serialPort->flush();
            serialPort->setDataTerminalReady(false);
            serialPort->setRequestToSend(true);
            msleep(100);
            serialPort->readAll();
            serialPort->setRequestToSend(false);
            msleep(10);
            serialPort->close();
        }
        delete serialPort;
        serialPort = 0;
    }

    exit(-1);
}

#define checkWaitForAck(msg) \
    ret = waitForAck(); \
    if (!ret) { qDebug() << msg << "failed at line" << __LINE__  << ", buffer:" << buffer.toHex(); bootModeExit(); return; }

#define checkWaitForAckMsecs(msg, msecs) \
    ret = waitForAck(msecs); \
    if (!ret) { qDebug() << msg << "failed at line" << __LINE__ << ", buffer:"<< buffer.toHex(); bootModeExit(); return; }

void Bootloader::run()
{
    bool ret;

    if (!openSerial()) {
        return;
    }

    bootModeEnter();

    if (!autoBaudrateSeq()) {
        bootModeExit();
        return;
    }

    writeCmd(GetVersionCommand);
    checkWaitForAck("Get version command");
    checkWaitForAck("Get version command");
    qDebug() << "Get version command" << buffer.toHex();
    if (!ret) {
        bootModeExit();
        return;
    }

    writeCmd(GetIDCommand);
    checkWaitForAck("Get ID command");
    checkWaitForAck("Get ID command");
    qDebug() << "Get ID command" << buffer.toHex();
    if (!ret) {
        bootModeExit();
        return;
    }

    int chipId = buffer.at(1) << 8 | buffer.at(2);
    int density;
    if (densityMap.contains(chipId)) {
        density = densityMap.value(chipId);
    } else {
        qDebug() << "Cannot find density by chip id:" << chipId;
        bootModeExit();
        return;
    }
    qDebug() << "Chip ID:" << chipId << ", Density:" << density;

    writeCmd(EraseMemoryCommand);
    checkWaitForAck("Erase memory command");
    writeCmd(0xff);
    checkWaitForAckMsecs("Erase memory command", 2000);

    QFile bin("C:/Users/cnh75547/Workspace/homesecurity/Objects/homesecurity.bin");
    ret = bin.open(QIODevice::ReadOnly);
    int fileSize = bin.size();
    int filePos = 0;
    qDebug() << "bin filesize:" << fileSize << ret;
    if (!ret)
        return;

    do {
        int bytes = fileSize - filePos;
        bytes = bytes > 256 ? 256 : bytes;
        bytes = ((bytes + 3) / 4) * 4;
        QByteArray fileBuf(256, 0xff);
        bin.seek(filePos);
        bin.read(fileBuf.data(), bytes);
        writeCmd(WriteMemoryCommand);
        checkWaitForAck("Write memory command");
        writeAddr(FlashBaseAddress + filePos);
        checkWaitForAck("Write memory command");
        writeData(fileBuf);
        checkWaitForAckMsecs("Write memory command", 2000);
        filePos += bytes;
        qDebug() << "file pos:" << filePos;
    } while (filePos < fileSize);

    bootModeExit();

    qDebug() << "programe finished";
}

char Bootloader::checkSum(const QByteArray &data)
{
    int size = data.size();
    char sum = 0;

    if (size == 0)
        sum = 0;
    else if (size == 1)
        sum = ~data.at(0);
    else {
        for (int i = 0; i < size; i++) {
            sum = sum ^ data.at(i);
        }
    }
    return sum;
}

qint64 Bootloader::write(char ch)
{
    return serialPort->write(QByteArray(1, ch));
}

qint64 Bootloader::writeCmd(char cmd)
{
    QByteArray array;
    array.append(cmd);
    array.append(~cmd);
    return serialPort->write(array);
}

qint64 Bootloader::writeAddr(quint32 addr)
{
    QByteArray array;
    array.append((addr >> 24) & 0xff);
    array.append((addr >> 16) & 0xff);
    array.append((addr >>  8) & 0xff);
    array.append((addr >>  0) & 0xff);
    array.append(checkSum(array));
    return serialPort->write(array);
}

qint64 Bootloader::writeBytesRead(char size)
{
    return writeCmd(size - 1);
}

qint64 Bootloader::writeData(const QByteArray &data)
{
    int size = data.size();
    QByteArray array;

    array.append(size - 1);
    array.append(data);
    array.append(checkSum(array));

    return serialPort->write(array);
}

bool Bootloader::waitForRead(int msec)
{
    return serialPort->waitForReadyRead(msec);
}

bool Bootloader::waitForAck(int msec)
{
    QTime timeout = QTime::currentTime().addMSecs(msec);
    buffer.clear();
    bool ack = false;

    forever {
        if (serialPort->bytesAvailable()) {
            QByteArray array = serialPort->read(1);
            char ch = array.at(0);
            if (ch == Ack) {
                ack = true;
                break;
            } else if (ch == Nack) {
                qDebug() << "Wait for Ack, Nack received";
                break;
            } else {
                buffer.append(ch);
            }
        } else {
            if (QTime::currentTime() > timeout) {
                qDebug() << "Wait for Ack timeout";
                break;
            } else {
                waitForRead(1);
            }
        }
    }

    return ack;
}

bool Bootloader::autoBaudrateSeq()
{
    bool ack = false;
    write(0x7f);

    if (waitForRead(5)) {
        QByteArray data = serialPort->readAll();
        if (data.at(0) == Ack) {
            ack = true;
        } else {
            qDebug() << "Auto-Baud rate sequence failed:" << data.toHex();
        }
    } else {
        qDebug() << "Auto-Baud rate sequence timeout";
    }

    return ack;
}

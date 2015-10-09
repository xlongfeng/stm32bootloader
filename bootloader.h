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
    QSerialPort *serialPort;
    QByteArray buffer;
    static QMap<int, int> densityMap;
};

#endif // BOOTLOADER_H

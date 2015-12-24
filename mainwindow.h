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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class QSerialPort;
class QTimer;
class Bootloader;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    void portChanged(const QString &text);
    void baudrateChanged(const QString &text);
    void suspendSerial();
    void readSerial();
    void writeSerial(int key);
    void writeSerial(const QString &data);
    void resetEnterAction();
    void resetExitAction();
    void openAction();
    void loadAction();
    void loadEnter();
    void loadExit();
    void loadProgress(int value);

private:
    QString portName() const;
    qint32 baudrate() const;
    QString filename() const;
    void openSerial(const QString &port, qint32 baudrate);
    void closeSerial();

private:
    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    bool suspend;
    QTimer *timer;
    Bootloader *bootloader;
};

#endif // MAINWINDOW_H

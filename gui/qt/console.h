/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QtCore/QObject>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtNetwork/QLocalServer>
QT_BEGIN_NAMESPACE
class QLocalSocket;
class QString;
class QThread;
QT_END_NAMESPACE

#include <cstdio>

namespace readline
{

class Worker : public QObject
{
    Q_OBJECT

public:
    Worker(QObject *parent = nullptr);
    ~Worker() override;

signals:
    void inputLine(const QString &line);

public slots:
    void start(const QString &name);
    void addHistoryLine(const QString &line);

private slots:
    void inputReady();

private:
    void connected(::FILE *socket);
    static Worker *sInstance;
    QTextCodec *mUtf8Codec;
    QTextCodec::ConverterState mUtf8State;
};

class Controller : public QObject
{
    Q_OBJECT

public:
    Controller(QObject *parent = nullptr);
    ~Controller();

    void connectToServer(const QLocalServer &server);

signals:
    void start(const QString &name);
    void inputLine(const QString &line);
    void addHistoryLine(const QString &line);

private:
    Worker mWorker;
    QThread *mThread = nullptr;
};

}

class Connection : public QObject
{
    Q_OBJECT

public:
    Connection(QLocalSocket *socket, QObject *parent = nullptr);

private slots:
    void readyRead();

private:
    QTextStream mStream;
};

class Console : public QObject
{
    Q_OBJECT

public:
    Console(QObject *parent = nullptr);
    ~Console() override;

signals:
    void inputLine(const QString &line);
    void addHistoryLine(const QString &line);

public slots:
    void outputText(const QString &text);
    void errorText(const QString &text);

private slots:
    void connection();

private:
    QLocalServer mServer;
    readline::Controller mReadline;
};

#endif

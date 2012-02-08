/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtAddOn.JsonStream module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QLocalSocket>
#include <QLocalServer>
#include "jsonserver.h"
#include "jsonstream.h"
#include "jsonuidauthority.h"

QT_USE_NAMESPACE_JSONSTREAM

Q_DECLARE_METATYPE(QJsonObject);

class Spy {
public:
    Spy(JsonServer *server)
        : addedSpy(server, SIGNAL(connectionAdded(const QString&)))
        , removedSpy(server, SIGNAL(connectionRemoved(const QString&)))
        , receivedSpy(server, SIGNAL(messageReceived(const QString&, const QJsonObject&)))
        , failedSpy(server, SIGNAL(authorizationFailed()))
    {}

    void waitAdded(int timeout=5000) {
        int oldCount = addedSpy.count();
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (addedSpy.count() > oldCount)
                break;
        }
    }

    void waitRemoved(int timeout=5000) {
        int oldCount = removedSpy.count();
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (removedSpy.count() > oldCount)
                break;
        }
    }

    void waitReceived(int timeout=5000) {
        int oldCount = receivedSpy.count();
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (receivedSpy.count() > oldCount)
                break;
        }
    }

    void waitFailed(int timeout=5000) {
        stopWatch.restart();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (failedSpy.count())
                break;
        }
    }

    QString     lastSender()  { return receivedSpy.last().at(0).toString(); }
    QJsonObject lastMessage() { return qvariant_cast<QJsonObject>(receivedSpy.last().at(1));}

    QTime      stopWatch;
    QSignalSpy addedSpy, removedSpy, receivedSpy, failedSpy;
};

class Child : public QObject {
    Q_OBJECT
public:
    Child(const QString& progname, const QStringList& arguments) {
        process = new QProcess;
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyread()));
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(finished(int, QProcess::ExitStatus)));
        connect(process, SIGNAL(stateChanged(QProcess::ProcessState)),
                SLOT(stateChanged(QProcess::ProcessState)));
        connect(process, SIGNAL(error(QProcess::ProcessError)),
                SLOT(error(QProcess::ProcessError)));
        process->start(progname, arguments);
        QVERIFY(process->waitForStarted(5000));
    }

    ~Child() {
        if (process)
            delete process;
        process = 0;
    }

    void waitForFinished() {
        if (process->state() == QProcess::Running)
            QVERIFY(process->waitForFinished(5000));
        delete process;
        process = 0;
    }

protected slots:
    void readyread() {
        QByteArray ba = process->readAllStandardOutput();
        QList<QByteArray> list = ba.split('\n');
        foreach (const QByteArray& s, list)
            if (s.size())
                qDebug() << "PROCESS" << s;
    }
    void stateChanged(QProcess::ProcessState state) {
        qDebug() << "Process state" << state;
    }
    void error(QProcess::ProcessError err) {
        qDebug() << "Process error" << err;
    }
    void finished( int exitcode, QProcess::ExitStatus status ) {
        qDebug() << Q_FUNC_INFO << exitcode << status;
    }

private:
    QProcess *process;
};

/****************************/
class BasicServer : public QObject {
    Q_OBJECT

public:
    BasicServer(const QString& socketname) : socket(0), stream(0) {
        QLocalServer::removeServer(socketname);
        server = new QLocalServer(this);
        connect(server, SIGNAL(newConnection()), SLOT(handleConnection()));
        QVERIFY(server->listen(socketname));
    }

    ~BasicServer() {
        QVERIFY(server);
        delete server;
        server = NULL;
    }

    void send(const QJsonObject& message) {
        QVERIFY(stream);
        stream->send(message);
    }

    void waitDisconnect(int timeout=5000) {
        QTime stopWatch;
        stopWatch.start();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= timeout)
                QFAIL("Timed out");
            if (!socket)
                break;
        }
    }

    EncodingFormat format() {
        return stream->format();
    }

private slots:
    void handleConnection() {
        socket = server->nextPendingConnection();
        QVERIFY(socket);
        stream = new JsonStream(socket);
        stream->setParent(socket);
        connect(socket, SIGNAL(disconnected()), SLOT(handleDisconnection()));
        connect(stream, SIGNAL(messageReceived(const QJsonObject&)),
                SIGNAL(messageReceived(const QJsonObject&)));
    }

    void handleDisconnection() {
        QVERIFY(socket);
        socket->deleteLater();
        socket = NULL;
        stream = NULL;
    }

signals:
    void messageReceived(const QJsonObject&);

private:
    QLocalServer *server;
    QLocalSocket *socket;
    JsonStream   *stream;
};

/****************************/

class tst_JsonStream : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void noAuthTest();
    void authTest();
    void authFail();
    void formatTest();
};

void tst_JsonStream::initTestCase()
{
    qRegisterMetaType<QJsonObject>();
}


void tst_JsonStream::noAuthTest()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    QVERIFY(server.listen(socketname));

    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitReceived();
    qDebug() << "Got note=" << spy.lastMessage() << "from" << spy.lastSender();
    QJsonObject msg;
    msg.insert("command", QLatin1String("exit"));
    QVERIFY(server.hasConnection(spy.lastSender()));
    QVERIFY(server.send(spy.lastSender(), msg));

    spy.waitRemoved();
    child.waitForFinished();
}

void tst_JsonStream::authTest()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    JsonUIDAuthority *authority = new JsonUIDAuthority;
    QVERIFY(server.listen(socketname, authority));

    authority->authorize(geteuid());
    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitReceived();
    qDebug() << "Got note=" << spy.lastMessage() << "from" << spy.lastSender();
    QJsonObject msg;
    msg.insert("command", QLatin1String("exit"));
    QVERIFY(server.hasConnection(spy.lastSender()));
    QVERIFY(server.send(spy.lastSender(), msg));

    spy.waitRemoved();
    child.waitForFinished();
    delete authority;
}

void tst_JsonStream::authFail()
{
    QString socketname = "/tmp/tst_socket";
    JsonServer server;
    Spy spy(&server);
    JsonUIDAuthority *authority = new JsonUIDAuthority;
    QVERIFY(server.listen(socketname, authority));

    // authority->authorize(geteuid());
    Child child("testClient/testClient", QStringList() << "-socket" << socketname);

    spy.waitFailed();
    child.waitForFinished();
    delete authority;
}


void tst_JsonStream::formatTest()
{
    QString socketname = "/tmp/tst_socket";

    QStringList formats = QStringList() << "qbjs" << "bson" << "utf8";

    foreach (const QString& format, formats) {
        BasicServer server(socketname);
        QSignalSpy spy(&server, SIGNAL(messageReceived(const QJsonObject&)));
        QTime stopWatch;

        Child child("testClient/testClient",
                    QStringList() << "-socket" << socketname << "-format" << format);

        stopWatch.start();
        forever {
            QTestEventLoop::instance().enterLoop(1);
            if (stopWatch.elapsed() >= 5000)
                QFAIL("Timed out");
            if (spy.count())
                break;
        }

        if (format == "qbjs")
            QVERIFY(server.format() == FormatQBJS);
        else if (format == "bson")
            QVERIFY(server.format() == FormatBSON);
        else if (format == "utf8")
            QVERIFY(server.format() == FormatUTF8);
        else
            QFAIL("Unrecognized format");

        QJsonObject msg = qvariant_cast<QJsonObject>(spy.last().at(0));
        QVERIFY(msg.value("text").isString() && msg.value("text").toString() == QLatin1String("Standard text"));
        QVERIFY(msg.value("int").isDouble() && msg.value("int").toDouble() == 100);
        QVERIFY(msg.value("float").isDouble() && msg.value("float").toDouble() == 100.0);
        QVERIFY(msg.value("true").isBool() && msg.value("true").toBool() == true);
        QVERIFY(msg.value("false").isBool() && msg.value("false").toBool() == false);
        QVERIFY(msg.value("array").isArray());
        QJsonArray array = msg.value("array").toArray();
        QVERIFY(array.size() == 3);
        QVERIFY(array.at(0).toString() == "one");
        QVERIFY(array.at(1).toString() == "two");
        QVERIFY(array.at(2).toString() == "three");
        QVERIFY(msg.value("object").isObject());
        QJsonObject object = msg.value("object").toObject();
        QVERIFY(object.value("item1").toString() == "This is item 1");
        QVERIFY(object.value("item2").toString() == "This is item 2");

        msg.insert("command", QLatin1String("exit"));
        server.send(msg);
        server.waitDisconnect();
        child.waitForFinished();
    }
}


QTEST_MAIN(tst_JsonStream)

#include "tst_jsonstream.moc"
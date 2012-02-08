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

#ifndef _JSON_SERVER_CLIENT_H
#define _JSON_SERVER_CLIENT_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QWeakPointer>

class QLocalSocket;

#include "jsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class JsonStream;
class JsonAuthority;

class Q_ADDON_JSONSTREAM_EXPORT JsonServerClient : public QObject
{
    Q_OBJECT
public:
    JsonServerClient(QObject *parent = 0);
    ~JsonServerClient();

    void start();
    void stop();

    void send(const QJsonObject &message);

    void setAuthority(JsonAuthority *authority);

    const QLocalSocket *socket() const;
    void setSocket(QLocalSocket *socket);

    QString identifier() const;

    QList<QJsonObject> messageQueue() const;

signals:
    void disconnected(const QString& identifier);
    void messageReceived(const QString& identifier, const QJsonObject& message);

    void authorized(const QString& identifier);
    void authorizationFailed();

private slots:
    void received(const QJsonObject& message);
    void handleDisconnect();

private:
    QString        m_identifier;
    QLocalSocket  *m_socket;
    JsonStream    *m_stream;
    QWeakPointer<JsonAuthority> m_authority;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // _JSON_SERVER_CLIENT_H
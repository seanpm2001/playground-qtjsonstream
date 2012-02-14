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

#ifndef JSON_UID_RANGE_AUTHORITY_H
#define JSON_UID_RANGE_AUTHORITY_H

#include <QHash>
#include <QLocalSocket>

#include "jsonauthority.h"
#include "jsonstream-global.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT JsonUIDRangeAuthority : public JsonAuthority
{
    Q_OBJECT
    Q_PROPERTY(uid_t minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(uid_t maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)

public:
    JsonUIDRangeAuthority(QObject *parent = 0);

    uid_t  minimum() const;
    void setMinimum(uid_t);

    uid_t  maximum() const;
    void setMaximum(uid_t);

    virtual AuthorizationRecord clientConnected(JsonStream *stream);
    virtual AuthorizationRecord messageReceived(JsonStream *stream, const QJsonObject &message);

signals:
    void minimumChanged();
    void maximumChanged();

private:
    uid_t m_minimum;
    uid_t m_maximum;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // JSON_UID_RANGE_AUTHORITY_H
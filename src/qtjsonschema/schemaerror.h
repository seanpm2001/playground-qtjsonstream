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

#ifndef SCHEMAERROR_H
#define SCHEMAERROR_H

#include "jsonschema-global.h"

#include <QJsonObject>
#include <QList>

QT_BEGIN_NAMESPACE_JSONSTREAM

class Q_ADDON_JSONSTREAM_EXPORT SchemaError
{
public:
    enum ErrorCode {
        NoError = 0,
        FailedSchemaValidation, // Invalid according to the schema
        InvalidSchemaOperation,
        InvalidObject,              // Unable to parse an incoming object
        FailedSchemaFileOpenRead,   // Schema file could not be opened or read from
        InvalidSchemaFolder,        // Schema folder does not exist
        InvalidSchemaLoading,       // Schema loading errors

        // Schema Errors
        SchemaWrongParamType = 100,
        SchemaWrongParamValue
    };

    SchemaError() {}
    SchemaError(const QJsonObject & _data) : m_data(_data) {}
    SchemaError(ErrorCode, const QString &);

    ErrorCode errorCode() const;
    QString errorString() const;
    QString errorSource() const;

    QList<SchemaError> subErrors() const;

    QJsonObject object() const { return m_data; }

    static const QString kCodeStr;
    static const QString kMessageStr;
    static const QString kSourceStr;
    static const QString kCounterStr;
    static const QString kErrorPrefixStr;

private:
    friend Q_ADDON_JSONSTREAM_EXPORT QDebug operator<<(QDebug, const SchemaError &);

    QJsonObject m_data;
};

QT_END_NAMESPACE_JSONSTREAM

#endif // SCHEMAERROR_H

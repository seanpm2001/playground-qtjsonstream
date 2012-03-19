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

#include <QDebug>
#include <QtEndian>
#include <QJsonDocument>
#include <QTextCodec>

#include "jsonbuffer_p.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "bson/qt-bson_p.h"

QT_BEGIN_NAMESPACE_JSONSTREAM

/*!
  \class JsonBuffer
  \brief The JsonBuffer class parses data received into appropriate Json messages

  The JsonBuffer class wraps an internal buffer.  As you append
  data, the JsonBuffer object parses for received Json objects and
  raises an appropriate signal for each received object.

  \warning This class does not support JSON arrays.  Arrays that are received will be ignored.
*/

/*!
  Construct a JsonBuffer object with the given \a parent
*/

JsonBuffer::JsonBuffer(QObject *parent)
    : QObject(parent)
    , mFormat(FormatUndefined)
    , mParserState(ParseNormal)
    , mParserDepth(0)
    , mParserOffset(0)
    , mParserStartOffset(-1)
    , mEmittedReadyRead(false)
    , mMessageAvailable(false)
    , mMessageSize(0)
    , mEnabled(true)
{
}

/*!
    \fn bool JsonBuffer::isEnabled() const

    Returns true if \l{readyReadMessage()} signal notifier is enabled; otherwise returns false.

    \sa setEnabled(), readyReadMessage()
*/

/*!
    \fn void JsonBuffer::setEnabled(bool enable)

    If \a enable is true, \l{readyReadMessage()} signal notifier is enabled;
    otherwise the notifier is disabled.

    The notifier is enabled by default, i.e. it emits the \l{readyReadMessage()}
    signal whenever a new message is ready.

    The notifier should normally be disabled while user is reading existing messages.

    \sa isEnabled(), readyReadMessage()
*/

/*!
    \fn int JsonBuffer::size() const

    Returns the size of the buffer in bytes.
*/

/*!
  Append the contents of a byte array \a data onto the buffer.
  During the execution of this function, the \l{readyReadMessage()}
  signal may be raised.
*/

void JsonBuffer::append(const QByteArray& data)
{
    mBuffer.append(data.data(), data.size());
    if (0 < size())
        processMessages();
}

/*!
  Append the \a data pointer with length \a len onto the JsonBuffer.
  During the execution of this function, the \l{readyReadMessage()}
  signal may be raised.
*/

void JsonBuffer::append(const char *data, int len)
{
    mBuffer.append(data, len);
    if (0 < size())
        processMessages();
}

/*!
  Copy data from a file descriptor \a fd into the buffer.
  This function tries to eliminate extra data copy operations.
  It assumes that the file descriptor is ready to read and
  it does not try to read all of the data.

  Returns the number of bytes read or -1 for an error condition.
 */

int JsonBuffer::copyFromFd(int fd)
{
    const int maxcopy = 1024;
    uint oldSize = mBuffer.size();
    mBuffer.resize(oldSize + maxcopy);
    int n = ::read(fd, mBuffer.data()+oldSize, maxcopy);
    if (n > 0) {
        mBuffer.resize(oldSize+n);
        processMessages();
    }
    else
        mBuffer.resize(oldSize);
    return n;
}

/*!
    Clear the contents of the buffer.
 */

void JsonBuffer::clear()
{
    mBuffer.clear();
    resetParser();
}

/*!
  \internal
*/

bool JsonBuffer::scanUtf( int c )
{
    switch (mParserState) {
    case ParseNormal:
        if ( c == '{' ) {
            if ( mParserDepth == 0 )
                mParserStartOffset = mParserOffset;
            mParserDepth += 1;
        }
        else if ( c == '}' && mParserDepth > 0 ) {
            mParserDepth -= 1;
            if ( mParserDepth == 0 ) {
                mParserOffset++;
                return true;
            }
        }
        else if ( c == '"' ) {
            mParserState = ParseInString;
        }
        break;
    case ParseInString:
        if ( c == '"' ) {
            mParserState = ParseNormal;
        } else if ( c == '\\' ) {
            mParserState = ParseInBackslash;
        }
        break;
    case ParseInBackslash:
        mParserState = ParseInString;
        break;
    }
    return false;
}

void JsonBuffer::resetParser()
{
    mParserState  = ParseNormal;
    mParserDepth  = 0;
    mParserOffset = 0;
    mParserStartOffset = -1;
    mMessageAvailable = false;
    mMessageSize = 0;
}

/*!
  \internal
*/

void JsonBuffer::processMessages()
{
    // do not process anything if disabled or if control is still inside readyReadMessage() slot
    if (mEnabled && !mEmittedReadyRead) {
        mEmittedReadyRead = true;
        if (messageAvailable()) {
            emit readyReadMessage();
        }
        mEmittedReadyRead = false;
    }
}

/*!
  \internal
*/
bool JsonBuffer::messageAvailable()
{
    if (mMessageAvailable) {
        // already found - no need to check again
        return true;
    }

    if (mFormat == FormatUndefined && mBuffer.size() >= 4) {
        if (strncmp("bson", mBuffer.constData(), 4) == 0)
            mFormat = FormatBSON;
        else if (QJsonDocument::BinaryFormatTag == *((uint *) mBuffer.constData()))
            mFormat = FormatQBJS;
        else if (mBuffer.at(0) == 0 &&
                 mBuffer.at(1) != 0 &&
                 mBuffer.at(2) == 0 &&
                 mBuffer.at(3) != 0 )
            mFormat = FormatUTF16BE;
        else if (mBuffer.at(0) != 0 &&
                 mBuffer.at(1) == 0 &&
                 mBuffer.at(2) != 0 &&
                 mBuffer.at(3) == 0 )
            mFormat = FormatUTF16LE;
        else
            mFormat = FormatUTF8;
    }

    switch (mFormat) {
    case FormatUndefined:
        break;
    case FormatUTF8:
        for (  ; mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            char c = mBuffer.at(mParserOffset);
            if (scanUtf(c)) {
                mMessageAvailable = true;
                return true;
            }
        }
        break;
    case FormatUTF16BE:
        for (  ; 2 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int16_t c = qFromBigEndian(reinterpret_cast<const int16_t *>(mBuffer.constData())[mParserOffset]);
            if (scanUtf(c)) {
                mMessageAvailable = true;
                return true;
            }
        }
        break;
    case FormatUTF16LE:
        for (  ; 2 * mParserOffset < mBuffer.size() ; mParserOffset++ ) {
            int16_t c = qFromLittleEndian(reinterpret_cast<const int16_t *>(mBuffer.constData())[mParserOffset]);
            if (scanUtf(c)) {
                mMessageAvailable = true;
                return true;
            }
        }
        break;
    case FormatBSON:
        if (mBuffer.size() >= 8) {
            qint32 message_size = qFromLittleEndian(((qint32 *)mBuffer.constData())[1]);
            if (mBuffer.size() >= message_size + 4) {
                mMessageSize = message_size;
                mMessageAvailable = true;
            }
        }
        break;
    case FormatQBJS:
        if (mBuffer.size() >= 12) {
            // ### TODO: Should use 'sizeof(Header)'
            qint32 message_size = qFromLittleEndian(((qint32 *)mBuffer.constData())[2]) + 8;
            if (mBuffer.size() >= message_size) {
                mMessageSize = message_size;
                mMessageAvailable = true;
            }
        }
        break;
    }
    return mMessageAvailable;
}

/*!
  \internal
*/
QJsonObject JsonBuffer::readMessage()
{
    QJsonObject obj;
    if (messageAvailable()) {
        switch (mFormat) {
        case FormatUndefined:
            break;
        case FormatUTF8:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset, mParserOffset - mParserStartOffset);
                obj = QJsonDocument::fromJson(msg).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset);
                resetParser();
            }
            break;
        case FormatUTF16BE:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset * 2, 2*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-16BE")->toUnicode(msg);
                obj = QJsonDocument::fromJson(s.toUtf8()).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset*2);
                resetParser();
            }
            break;
        case FormatUTF16LE:
            if (mParserStartOffset >= 0) {
                QByteArray msg = rawData(mParserStartOffset * 2, 2*(mParserOffset - mParserStartOffset));
                QString s = QTextCodec::codecForName("UTF-16LE")->toUnicode(msg);
                obj = QJsonDocument::fromJson(s.toUtf8()).object();
                // prepare for the next
                mBuffer.remove(0, mParserOffset*2);
                resetParser();
            }
            break;
        case FormatBSON:
            if (mMessageSize > 0) {
                QByteArray msg = rawData(4, mMessageSize);
                obj = QJsonDocument::fromVariant(BsonObject(msg).toMap()).object();
                mBuffer.remove(0, mMessageSize+4);
                mMessageSize = 0;
            }
            break;
        case FormatQBJS:
            if (mMessageSize > 0) {
                QByteArray msg = rawData(0, mMessageSize);
                obj = QJsonDocument::fromBinaryData(msg).object();
                mBuffer.remove(0, mMessageSize);
                mMessageSize = 0;
            }
            break;
        }
        mMessageAvailable = false;
    }
    return obj;
}

/*!
  Return the current encoding format used by the receive buffer
*/

EncodingFormat JsonBuffer::format() const
{
    return mFormat;
}

/*!
    \fn void JsonBuffer::readyReadMessage()

    This signal is emitted once every time new data is appended to the buffer
    and a message is ready. \b readMessage() should be used to retrieve a message
    and \b messageAvailable() to check for next available messages.
    The client code may look like this:

    \code
    ...
    connect(jsonbuffer, SIGNAL(readyReadMessage()), this, SLOT(processMessages()));
    ...

    void Foo::processMessages()
    {
        while (jsonbuffer->messageAvailable()) {
            QJsonObject obj = jsonbuffer->readMessage();
            <process message>
        }
    }
    \endcode

    \b readyReadMessage() is not emitted recursively; if you reenter the event loop
    inside a slot connected to the \b readyReadMessage() signal, the signal will not
    be reemitted.
*/

#include "moc_jsonbuffer_p.cpp"

QT_END_NAMESPACE_JSONSTREAM

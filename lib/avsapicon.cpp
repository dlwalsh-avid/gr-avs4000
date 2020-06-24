//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "avsapicon.h"

#define AVSAPI_PORT     12900

//static bool verbose=true;
static bool verbose=false;

AVSAPICon::AVSAPICon(const QString &hostname,quint16 dn)
{
    this->hostname=hostname;
    this->dn=dn;
    this->socket=nullptr;
}

bool AVSAPICon::connect()
{
    if (socket) {
//        qWarning("Already connected");
        disconnect();
    }
    socket=new QTcpSocket();
    socket->connectToHost(hostname,AVSAPI_PORT+dn);
    bool connected=socket->waitForConnected();
    if (!connected)
        qWarning("Failed to connect.");
    return connected;
}

void AVSAPICon::disconnect()
{
    if (socket) {
        socket->close();
        socket->deleteLater();
        socket=nullptr;
    }
}

bool AVSAPICon::sendREQ(const QString &req)
{
    if (socket && socket->state()==QTcpSocket::ConnectedState) {
        QByteArray bytes=req.toUtf8();
        if (verbose)
            qDebug("REQ: %s",qPrintable(QString(bytes)));
        int len=bytes.length();
        while (bytes.length()>0) {
            bytes.append('\n');
            qint64 rval=socket->write(bytes);
            if (rval<0) {
                qWarning("socket error='%s'",qPrintable(socket->errorString()));
                return false;
            }
            bytes.remove(0,int(rval));
        }
        bool written=socket->waitForBytesWritten(1000);
        if (written) {
            if (verbose)
                qDebug("Sent %u bytes",len);
        } else
            qWarning("failed to send %d bytes",len);
        return written;
    } else {
        qWarning("Not connected");
        connect();
    }
    return false;
}

QString AVSAPICon::recvRSP()
{
    if (socket && socket->isValid()) {
        QByteArray bytes=socket->readLine(128*1024);
        if (bytes.length()==0) // if empty try again
            bytes=socket->readLine(128*1024);
        QString rsp=QString(bytes);
        if (verbose)
            qDebug("RSP: %s",qPrintable(rsp));
        return rsp;
    } else
        qWarning("Invalid socket");
    return QString();
}


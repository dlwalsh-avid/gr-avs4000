//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef AVSAPICON_H
#define AVSAPICON_H

#include <QObject>
#include <QTcpSocket>

class AVSAPICon : public QObject
{
    Q_OBJECT
public:
    AVSAPICon(const QString &hostname,quint16 dn);

signals:

public slots:
    bool connect();
    void disconnect();
    bool sendREQ(const QString &req);
    QString recvRSP();

    QString text() const { return "foobar"; }
protected:
    QString hostname;
    quint16 dn;
    QTcpSocket *socket;
    bool connected;
};

#endif // AVSAPICON_H

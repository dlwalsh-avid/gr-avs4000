//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef TCPRXSIGCLIENT_H
#define TCPRXSIGCLIENT_H

#include "RxSignalClient.h"
#include "SocketRxBuffer.h"
#include "v49packet.h"
#include <QString>

class TcpRxSignalClient : public RxSignalClient
{
    Q_OBJECT
public:
    explicit TcpRxSignalClient(bool useVita49,const QString &hostname,quint16 port);
    ~TcpRxSignalClient();
    static bool verbose;
    static bool GetVerbose() { return verbose; }
    static void SetVerbose(bool val) { verbose=val; }
    virtual bool IsValid() const { return (sock>=0); }
    virtual void Flush();
    virtual bool Done() const { return buffer.Done(); }
    virtual bool IsConnected() const { return (sock>=0); }
    virtual bool WaitForReadyReceive(quint32 msTimeout) { return buffer.WaitForReadyReceive(msTimeout); }
    virtual quint32 Receive(quint8 *data, quint32 len);
    virtual quint32 ReceiveSamples(quint32 *samples, quint32 count,QList<TimeTag> &tList);

signals:

public slots:

protected:
    QString hostname;
    quint16 port;
    int sock;
    SocketRxBuffer buffer;
    bool useV49;
    V49Packet packet;
    quint32 partial;   // partial count
};

#endif // TCPRXSIGCLIENT_H

//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef TCPTXSIGCLIENT_H
#define TCPTXSIGCLIENT_H

#include "TxSignalClient.h"
#include "SocketTxBuffer.h"
#include <QString>
#include "v49packet.h"

class TcpTxSignalClient : public TxSignalClient
{
    Q_OBJECT
public:
    explicit TcpTxSignalClient(quint8 dn, bool useVita49,
                               const QString &hostname, quint16 port);
    ~TcpTxSignalClient();
    static bool verbose;
    static bool GetVerbose() { return verbose; }
    static void SetVerbose(bool val) { verbose=val; }
    virtual bool IsValid() const { return (sock>=0); }
    virtual void Flush();
    virtual bool Done() const { return buffer.Done(); }
    virtual bool IsConnected() const { return IsValid(); }
    virtual bool WaitForReadySend(quint32 msTimeout) { return buffer.WaitForReadySend(msTimeout); }
    virtual quint32 Send(const quint8 *data, quint32 len);

signals:

public slots:
protected:
    QString hostname;
    quint16 port;
    SocketTxBuffer buffer;
    int sock;
    bool useV49;
    V49Packet packet;
    quint32 plen;   // partial len
    quint16 nextFrameCount;
    quint8 nextPacketCount;
    quint8 dn;
};

#endif // TCPTXSIGCLIENT_H

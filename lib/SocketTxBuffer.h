//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef SOCKETTXBUFER_H
#define SOCKETTXBUFER_H

#include <QObject>
#include <QAbstractSocket>

#include "BlockFifo.h"

#include <thread>

class SocketTxBuffer;

///
/// \brief The SocketTxBuffer class
/// SocketTxBufer Class writes data from buffer to a socket
///
class SocketTxBuffer : public QObject
{
    Q_OBJECT
public:
    explicit SocketTxBuffer(bool verbose,quint32 blkSize=32*1024,
                            quint32 blkCount=64);
    ~SocketTxBuffer();
    bool Done() const { return done; }
    bool Empty() { return fifo.Empty(); }
    bool Full() { return fifo.Full(); }
    void Reset();
    quint64 Transferred() const { return transferred; }
    quint64 Sent() const { return sent; }   // number of bytes sent through send()
    void Attach(int fd);
    void Detach();
    bool WaitForReadySend(quint32 msTimeout) { return fifo.WaitNotFull(msTimeout); }
    quint32 Send(const quint8 *data,quint32 len);
    quint8 *SendStart(quint32 &maxAmt,quint32 msTimeout=0) { return fifo.WriteStart(maxAmt,msTimeout); }
    void SendComplete(quint32 amt) { fifo.WriteComplete(amt); transferred+=amt; }

signals:
    void Error();
    void Closed();

public slots:

protected:
    void Pump();
    void WriteData();
    int fd;
    quint64 transferred;
    quint64 sent;
    BlockFifo fifo;
    bool done;
    bool error;
    bool verbose;
    std::thread thread;
};

#endif

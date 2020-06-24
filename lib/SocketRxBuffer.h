//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef SOCKETRXBUFER_H
#define SOCKETRXBUFER_H

#include <QObject>
#include <QAbstractSocket>
#include "BlockFifo.h"

#include <thread>

///
/// \brief The SocketRxBuffer class
/// SocketRxBufer Class reads data from a socket and buffers it
///
class SocketRxBuffer : public QObject
{
    Q_OBJECT
public:
    explicit SocketRxBuffer(bool verbose,quint32 blkSize=32*1024,
                            quint32 blkCount=64);
    ~SocketRxBuffer();
    bool Done() const { return done; }
    bool Empty() const { return fifo.Empty(); }
    bool Full() const { return fifo.Full(); }
    bool HalfFull() const { return fifo.HalfFull(); }

    void Reset();
    quint64 Transferred() const { return transferred; }
    void Attach(int fd);
    void Detach();
    bool WaitForReadyReceive(quint32 msTimeout) { return fifo.WaitNotEmpty(msTimeout); }
    quint32 Receive(quint8 *data,quint32 len);
    const quint8 *ReceiveStart(quint32 &maxAmt,quint32 msTimeout=0) { return fifo.ReadStart(maxAmt,msTimeout); }
    void ReceiveComplete(quint32 amt) { fifo.ReadComplete(amt); transferred+=amt; }

signals:
    void Error();
    void Closed();

public slots:

protected slots:

protected:
    void Pump();
    void ReadData();
    int fd;
    quint64 transferred;
    quint64 received;
    BlockFifo fifo;
    bool done;
    bool error;
    bool verbose;
    std::thread thread;
};

#endif

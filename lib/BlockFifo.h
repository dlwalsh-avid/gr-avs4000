//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef BLOCKFIFO_H
#define BLOCKFIFO_H

#include <QObject>
#include <QWaitCondition>
#include <mutex>
#include <condition_variable>

#define USE_WC  1

class BlockFifo
{
public:
    explicit BlockFifo(quint32 blkSize=256*1024,quint32 blkCount=16);
    ~BlockFifo();
    typedef struct {
        quint32 len;    // how much data was in the buffer
        quint32 read;   // amount of data in buffer that has been read
                        // real amount of data available is len-used
        quint32 size;   // size of buffer
        quint8 *buf;
        quint32 id;
    } Block;
    bool Empty() const { return (head==tail); }
    bool Full() const { return (PINC(head)==tail); }
    bool HalfFull() const { return (count>=(blkCount/2)); }
    quint32 Count() const { return count; }
    void Reset();

    quint8 *WriteStart(quint32 &maxAmt,quint32 msTimeout=0);
    void WriteComplete(quint32 amt);
    quint32 Write(const quint8 *buf,quint32 len,quint32 msTimeout=0);

    const quint8 *ReadStart(quint32 &maxAmt, quint32 msTimeout=0);
    void ReadComplete(quint32 amt);
    quint32 Read(quint8 *buf,quint32 len,quint32 msTimeout=0);

    bool WaitNotFull(quint32 msTimeout=1000);
    bool WaitNotEmpty(quint32 msTimeout=1000);
    // Test to write COUNT number of 32-bit words to fifo
    // while also reading and checking the values
    quint32 Test(quint64 size);
protected:
    quint32 PINC(quint32 ptr) const { return (ptr+1)%blkCount; }
    void Reset(Block *blk) {
        Q_ASSERT(blk);
        blk->len=blk->read=0;
    }
    quint32 blkSize,blkCount;
    quint32 head,tail;
    quint32 count;
    Block **blk;
    Block *inBlk;
    Block *outBlk;
    std::mutex mutex;
#if USE_WC
    std::condition_variable notEmpty;
    std::condition_variable notFull;
#endif
};

#endif

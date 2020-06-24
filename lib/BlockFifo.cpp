//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "BlockFifo.h"

#include <stdlib.h>
#include <QElapsedTimer>
#include <QThread>
#include <unistd.h>
#include <chrono>

BlockFifo::BlockFifo(quint32 blkSize,quint32 blkCount)
{
    this->blkSize=blkSize;
    this->blkCount=blkCount;
    this->head=this->tail=0;
    this->count=0;
    this->outBlk=this->inBlk=nullptr;
    blk=new Block*[blkCount];
    for (quint32 i=0;i<blkCount;i++) {
        Block *b=new Block();
        b->size=blkSize;
        b->buf=reinterpret_cast<quint8*>(malloc(blkSize));
        memset(b->buf,0,blkSize);
        b->id=i;
        Reset(b);
        blk[i]=b;
    }
}

BlockFifo::~BlockFifo()
{
    Reset();    // reset buffers
    // free the buffers;
    for (quint32 i=0;i<blkCount;i++) {
        if (blk[i]) {
            if (blk[i]->buf)
                free(blk[i]->buf);
            delete blk[i];
        }
    }
    delete blk;
}


void BlockFifo::Reset()
{
    std::unique_lock<std::mutex> locker(mutex);
    this->head=this->tail=0;
    this->count=0;
    outBlk=inBlk=nullptr;
    for (quint32 i=0;i<blkCount;i++) {
        if (blk[i])
            Reset(blk[i]);
    }
}

bool BlockFifo::WaitNotFull(quint32 msTimeout)
{
#if !USE_WC
    mutex.lock();
    if (!Full()) {
        mutex.unlock();
        return true;
    }
    /// Looks like the QWaitCondition may have some sort of leak
    /// The following achieves the same thing but uses a bit more CPU
    /// May need to look at using C++11 STD condition_variable
    for (quint32 i=0;i<msTimeout;i++) {
        if (!Full()) {
            mutex.unlock();
            return true;
        }
        mutex.unlock();
        usleep(100);
//        QThread::msleep(1);
        mutex.lock();
    }
    mutex.unlock();
    return false;
#else
    std::unique_lock<std::mutex> locker(mutex);
    if (!Full()) return true;
    if (msTimeout>0) {
        while (notFull.wait_for(locker,
                                std::chrono::milliseconds(msTimeout))!=std::cv_status::timeout)
            if (!Full()) break;

    }
    return !Full();
#endif
}

bool BlockFifo::WaitNotEmpty(quint32 msTimeout)
{
#if !USE_WC
    mutex.lock();
    if (!Empty()) {
        mutex.unlock();
        return true;
    }
    /// Looks like the QWaitCondition may have some sort of leak
    /// The following achieves the same thing but uses a bit more CPU
    /// May need to look at using C++11 STD condition_variable
    for (quint32 i=0;i<msTimeout;i++) {
        if (!Empty()) {
            mutex.unlock();
            return true;
        }
        mutex.unlock();
        usleep(100);
//        QThread::msleep(1);
        mutex.lock();
    }
#else
    std::unique_lock<std::mutex> locker(mutex);
    if (!Empty()) return true;
    if (msTimeout>0) {
        while (notEmpty.wait_for(locker,
                                std::chrono::milliseconds(msTimeout))!=std::cv_status::timeout)
            if (!Empty()) break;
        return !Empty();
    }
#endif
    return false;
}


quint8 *BlockFifo::WriteStart(quint32 &maxAmt, quint32 msTimeout)
{
    if (outBlk) {
        std::unique_lock<std::mutex> locker(mutex);
        maxAmt=outBlk->size-outBlk->len;
        return &outBlk->buf[outBlk->len];
    } else if (WaitNotFull(msTimeout)) {
        std::unique_lock<std::mutex> locker(mutex);
        outBlk=blk[head];
        Q_ASSERT(outBlk);
        Reset(outBlk);
        maxAmt=outBlk->size;
        return outBlk->buf;
    } else
        return nullptr;
}

void BlockFifo::WriteComplete(quint32 amt)
{
    std::unique_lock<std::mutex> locker(mutex);
    if (outBlk) {
        if (amt>(outBlk->size-outBlk->len)) {
            QString str;
            str.sprintf("! amt=%d (0x%08X) size=%d len=%d",
                     amt,amt,outBlk->size,outBlk->len);
            qCritical("%s",qPrintable(str));
        }
        Q_ASSERT(amt<=(outBlk->size-outBlk->len));
        outBlk->len+=amt;
        Q_ASSERT(outBlk->len<=outBlk->size);
        if (amt==0 || outBlk->len==outBlk->size) {
            count++;
        #if USE_WC
        //    bool wasEmpty=Empty();
        #endif
            head=PINC(head);
        #if USE_WC
        //    if (wasEmpty)
            notEmpty.notify_one();
        #endif
            outBlk=nullptr;
        }
    } else
        qWarning("outBlk==NULL!!  head=%d tail=%d count=%d blkCount=%d",
                 head,tail,count,blkCount);
}

const quint8 *BlockFifo::ReadStart(quint32 &maxAmt,quint32 msTimeout)
{
    if (inBlk) {
        std::unique_lock<std::mutex> locker(mutex);
        Q_ASSERT(count>0);
        maxAmt=inBlk->len-inBlk->read;
        return &inBlk->buf[inBlk->read];
    } else if (WaitNotEmpty(msTimeout)) {
        std::unique_lock<std::mutex> locker(mutex);
        Q_ASSERT(count>0);
        inBlk=blk[tail];
        maxAmt=inBlk->len;
        return inBlk->buf;
    } else
        return nullptr;
}

void BlockFifo::ReadComplete(quint32 amt)
{
    std::unique_lock<std::mutex> locker(mutex);
    if (inBlk) {
        inBlk->read+=amt;
        Q_ASSERT(count>0);
        Q_ASSERT(inBlk->len<=inBlk->size);
        Q_ASSERT(inBlk->read<=inBlk->len);
        if (inBlk->len==inBlk->read) {
            count--;
            tail=PINC(tail);
            notFull.notify_one();
            inBlk=nullptr;
        }
    } else
        qWarning("inBlk is null!");
}

quint32 BlockFifo::Write(const quint8 *buf, quint32 len,quint32 msTimeout)
{
    quint32 written=0;
    Q_ASSERT(len>0);
    while (written<len) {
        quint32 max;
        quint8 *p=WriteStart(max,msTimeout);
        if (p==nullptr) break;
        Q_ASSERT(max>0 && max<=this->blkSize);
        quint32 amt=(len-written)>max?max:(len-written);
        Q_ASSERT(amt>0 && amt<=this->blkSize);
        memcpy(p,&buf[written],amt);
        WriteComplete(amt);
        written+=amt;
    }
    return written;
}

quint32 BlockFifo::Read(quint8 *buf, quint32 len,quint32 msTimeout)
{
    quint32 read=0;
    while (read<len) {
        quint32 max;
        const quint8 *p=ReadStart(max,msTimeout);
        if (p==nullptr) break;
        quint32 amt=(len-read)>max?max:(len-read);
        memcpy(&buf[read],p,amt);
        ReadComplete(amt);
        read+=amt;
    }
    return read;
}

quint32 BlockFifo::Test(quint64 size)
{
    quint32 count=quint32(size/sizeof(quint32));
    quint32 errors=0;
    quint32 read=0;
    quint32 wrSeed=0;
    quint32 rdSeed=0;
    const quint32 BufSize=128*1024;
    quint32 *wrBuf=new quint32[BufSize];
    quint32 *rdBuf=new quint32[BufSize];

    /// Need a method that will test FIFO
    /// Use 32-bit counter
    /// Need to do random sizes for reads & writes
    while (read<count) {
        // Write some data
        quint32 wrLen=quint32(rand())%BufSize+1;
        for (quint32 i=0;i<wrLen;i++)
            wrBuf[i]=wrSeed+i;
        quint32 wrAmt=Write(reinterpret_cast<quint8*>(wrBuf),wrLen*sizeof(quint32));
        qDebug("Wrote %u bytes",wrAmt);
        quint32 wrWords=wrAmt/sizeof(quint32);
        wrSeed+=wrWords;
        quint32 rdLen=quint32(rand())%BufSize+1;
        if (rdLen>(count-read))
            rdLen=count-read;
        quint32 rdAmt=Read(reinterpret_cast<quint8*>(rdBuf),rdLen*sizeof(quint32));
        quint32 rdWords=rdAmt/sizeof(quint32);
        qDebug("Read %u %u/%u",rdAmt,read+rdWords,count);
        for (quint32 i=0;i<rdWords;i++) {
            if (rdSeed!=rdBuf[i]) {
                errors++;
                qDebug("0x%08lX: 0x%08X != 0x%08X",
                       (read+i)*sizeof(quint32),rdSeed,rdBuf[i]);
                rdSeed=rdBuf[i];
            }
            rdSeed++;
        }
        read+=rdWords;
    }
    delete [] wrBuf;
    delete [] rdBuf;
    return errors;
}


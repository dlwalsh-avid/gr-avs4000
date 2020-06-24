//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <assert.h>

#include "SocketTxBuffer.h"
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <memory.h>

#include <netinet/in.h>
#include <netinet/tcp.h>


SocketTxBuffer::SocketTxBuffer(bool verbose,quint32 blkSize,quint32 blkCount) :
    QObject(nullptr),fifo(blkSize,blkCount)
{
    this->verbose=verbose;
    this->transferred=this->sent=0LL;
    this->fd=-1;    // not connected
    this->done=false;
    this->error=false;
}

SocketTxBuffer::~SocketTxBuffer()
{
    Detach();
}

void SocketTxBuffer::Reset()
{
//    qDebug("Reset...");
    this->transferred=this->sent=0LL;
    fifo.Reset();
}

void SocketTxBuffer::Attach(int fd)
{
    Detach();
//    qDebug("fd=%d",fd);
    this->fd=fd;
    done=false;
    error=false;
    Reset();
    auto func=std::bind(&SocketTxBuffer::Pump,this);
    this->thread=std::thread(func);
}

void SocketTxBuffer::Detach()
{
    done=true;
    if (thread.joinable())
        thread.join();
    this->fd=-1;
}

void SocketTxBuffer::Pump()
{
    if (fd<0) {
        qWarning("Failed to start pump: No valid socket");
        return;
    }
    int sndBufSize=0;
    socklen_t optlen;
    int rval;
    sndBufSize=1*1024*1024;
    optlen=sizeof(sndBufSize);
    rval=setsockopt(fd,SOL_SOCKET,SO_SNDBUF,&sndBufSize,optlen);
    if (rval<0)
        qWarning("setsockopt failed.");
    int flags=fcntl(fd, F_GETFL, 0);
//    qDebug("send Buf=0x%04X  flags=0x%04X",sndBufSize,flags);
    if (flags<0)
        qWarning("Failed to get fnctl flags");
    else if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        qWarning("Failed to set O_NONBLOCK");

    fd_set wrSet;
    struct timeval tv;
    while (!done) {
        tv.tv_sec=0;
        tv.tv_usec=10000;   // 10ms
        FD_ZERO(&wrSet);
        FD_SET(fd,&wrSet);
        int rval=select(fd+1,nullptr,&wrSet,nullptr,&tv);
        if (rval>0) {
            if (FD_ISSET(fd,&wrSet))
                WriteData();
        }
    }
//    if (ready.count()>0 && verbose)
//        qDebug("Pump exiting... pool=%d ready=%d",pool.count(),ready.count());
//    qDebug("PumpExit");
}

void SocketTxBuffer::WriteData()
{
    while (!done) {
        if (fd<0) break;
        quint32 maxAmt=0;
        const quint8 *p=fifo.ReadStart(maxAmt,100);
        if (p==nullptr) {
            // what should we do?
//            QThread::yieldCurrentThread();
//            emit SpaceAvailable();
//            QThread::msleep(1);
//            break;
            continue;
        }
        ssize_t rval;
//        rval=::send(fd,p,maxAmt,MSG_DONTWAIT);
        rval=::send(fd,p,maxAmt,0);
//        qDebug("amt=%d rval=%ld count=%d",maxAmt,rval,fifo.Count());
        if (rval<0 && errno==EAGAIN) {
//                qDebug("EAGAIN... count=%d sent=%lld",fifo.Count(),sent);
            std::this_thread::yield();
            continue;
        } else if (rval<0) {
//            if (verbose)
                qWarning("send returned rval=%ld sent=%lld/%lld errno=%s",
                        rval,sent,transferred,strerror(errno));
            done=true;
            emit Closed();
            break;
        } else if (rval==0) {
            // EOF
            qDebug("EOF");
            done=true;
            emit Closed();
            break;
        } else {
//            if ((rval%2)!=0) qWarning("Not 16-bit aligned");
//            qDebug("sent %d/%lld/%lld bytes",quint32(rval),sent,transferred);
            fifo.ReadComplete(quint32(rval));
            sent+=quint64(rval);
            break;
        }
    }
}


quint32 SocketTxBuffer::Send(const quint8 *data,quint32 len)
{
    quint32 queued=0;
    while (!done && queued<len) {
        quint32 amt=len-queued;
        quint32 rval=fifo.Write(&data[queued],amt,1000);
        if (rval==0) {
            qDebug("rval==0");
//            QThread::yieldCurrentThread();
//            QThread::msleep(1);
            break;
        }
        queued+=rval;
    }
    transferred+=queued;
//    qDebug("queued %d bytes DONE=%d",queued,done);
    return queued;
}


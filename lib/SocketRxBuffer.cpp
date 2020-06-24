//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <assert.h>

#include "SocketRxBuffer.h"
#include <QEventLoop>
#include <QTimer>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <functional>

SocketRxBuffer::SocketRxBuffer(bool verbose,quint32 blkSize,quint32 blkCount) :
    QObject(nullptr),fifo(blkSize,blkCount)
{
    this->verbose=verbose;
    this->transferred=0;
    this->fd=-1;    // not connected
    this->done=false;
    this->error=false;
}

SocketRxBuffer::~SocketRxBuffer()
{
    Detach();
}


void SocketRxBuffer::Reset()
{
//    qDebug("Reset...");
    transferred=received=0LL;
    fifo.Reset();
}

void SocketRxBuffer::Attach(int fd)
{
    Detach();
//    qDebug("fd=%d",fd);
    this->fd=fd;
    Reset();
    done=false;
    error=false;
    int rcvBufSize=0;
    socklen_t optlen=sizeof(rcvBufSize);
    int rval;
    rcvBufSize=1*1024*1024;
    rval=setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&rcvBufSize,optlen);
    optlen=sizeof(rcvBufSize);
    rval=getsockopt(fd,SOL_SOCKET,SO_RCVBUF,&rcvBufSize,&optlen);
    if (rval<0)
        qWarning("setsockopt failed.");
    int flags=fcntl(fd, F_GETFL, 0);
//    qDebug("send Buf=0x%04X  flags=0x%04X",sndBufSize,flags);
    if (flags<0)
        qWarning("Failed to get fnctl flags");
    else if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        qWarning("Failed to set O_NONBLOCK");
//    qDebug("recv Buf=0x%04X",rcvBufSize);
    auto func=std::bind(&SocketRxBuffer::Pump,this);
    this->thread=std::thread(func);
}

void SocketRxBuffer::Detach()
{
    done=true;
    if (thread.joinable())
        thread.join();
}

void SocketRxBuffer::Pump()
{
    if (fd<0) {
        qWarning("Failed to start pump: No valid socket");
        return;
    }
    quint32 pcount=0;
    fd_set rdSet;
    struct timeval tv;
    while (!done) {
        pcount++;
        tv.tv_sec=0;
        tv.tv_usec=10000;   // 10ms
        FD_ZERO(&rdSet);
        FD_SET(fd,&rdSet);
        int rval=select(fd+1,&rdSet,nullptr,nullptr,&tv);
        if (rval>0) {
            if (FD_ISSET(fd,&rdSet))
                ReadData();
        }
    }
//    qDebug("Pump exiting... fifo.count=%d",fifo.Count());
}

void SocketRxBuffer::ReadData()
{
//    qDebug("ReadData...");
    while (!done) {
        if (fd<0) break;    // invalid socket
        quint32 maxAmt;
        quint8 *p=fifo.WriteStart(maxAmt,100);
        if (p==nullptr) {
//            qDebug("Wait...");
            break;
        }
        ssize_t rval;
        Q_ASSERT(maxAmt>0);
        rval=::recv(fd,p,maxAmt,0);
        if (rval<0 && errno==EAGAIN) {
//            qDebug("0x%08llX EAGAIN: blk.len=0x%02X count=%d",
//                   transferred,maxAmt,fifo.Count());
                break;
        } else if (rval<0) {
//            if (verbose)
                qWarning("read returned rval=%ld errno=%d transferred=%lld errno=%s",
                         rval,errno,transferred,strerror(errno));
            done=true;
            emit Closed();
            break;
        } else if (rval==0) {
            // EOF
//            qDebug("EOF");
            done=true;
            emit Closed();
            break;
        } else {
//            if (rval<maxAmt)
//                qDebug("recv: 0x%08llX %ld/%u bytes count=%d",
//                       transferred,rval,maxAmt,fifo.Count());
            transferred+=quint64(rval);
            if (rval<0)
                qDebug("rval=%ld",rval);
            fifo.WriteComplete(quint32(rval));
        }
    }
}

quint32 SocketRxBuffer::Receive(quint8 *data,quint32 len)
{
//    qDebug("recv: len=%d",len);
    // Receive will fill the supplied buffer with the available
    // data.  The number of bytes read may be less than the
    // length of the buffer.
    quint32 read=0;
    while (!done && read<len) {
        quint32 amt=len-read;
        quint32 rval=fifo.Read(&data[read],amt,100);
        if (rval==0) {
//            qDebug("rval==0");
//            QThread::yieldCurrentThread();
//            QThread::msleep(1);
            break;
        }
        read+=rval;
    }
    return read;
}



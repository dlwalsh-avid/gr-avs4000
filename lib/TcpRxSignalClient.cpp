//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "TcpRxSignalClient.h"
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <QCoreApplication>

bool TcpRxSignalClient::verbose=false;

TcpRxSignalClient::TcpRxSignalClient(bool useV49,const QString &hostname,quint16 port):
    RxSignalClient(),buffer(verbose)
{
    // LOOK HERE!! need to fix to actually use hostname
    this->useV49=useV49;
    this->hostname=hostname;
    this->port=port;
    this->sock=-1;
    this->partial=0;
    sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock>=0) {
        const char *A="127.0.0.1";
        struct sockaddr_in addr;
        memset(&addr,0,sizeof(addr));
        addr.sin_addr.s_addr=inet_addr(A);
        addr.sin_family=AF_INET;
        addr.sin_port=htons(port);
        if (::connect(sock,reinterpret_cast<struct sockaddr *>(&addr),sizeof(addr)))
            qWarning("Failed to connect to %s:%d - %s",A,port,strerror(errno));
        else {
            buffer.Attach(sock);
            if (verbose)
                qDebug("Connected to %s:%d",A,port);
        }
    } else
        qWarning("Failed to create socket");
}

TcpRxSignalClient::~TcpRxSignalClient()
{
    if (sock>=0) {
        if (verbose)
            qDebug("Disconnect: socket=%d",sock);
        buffer.Detach();
        close(sock);
        sock=-1;
        if (verbose)
            qDebug("Disconnected.");
    }
}

void TcpRxSignalClient::Flush()
{
    if (verbose)
        qDebug("Flush...");
    buffer.Reset();
}

quint32 TcpRxSignalClient::Receive(quint8 *data,quint32 len)
{
    quint32 scount=ReceiveSamples(reinterpret_cast<quint32*>(data),len/sizeof(quint32));
    return scount*sizeof(quint32);
}

quint32 TcpRxSignalClient::ReceiveSamples(quint32 *samples,quint32 count,TimeTag *t)
{
    quint32 recv=0;
    //qDebug("ReceiveSamples: count=%d v49=%d",count,useV49);
    if (useV49) {
        if (t) t->valid=false;
        const quint32 pktSize=0x2000;   // 8KB packets
        while (recv<count) {
            quint32 amt=count-recv;
            if (partial==0) {
                if (buffer.WaitForReadyReceive(100)) {
                    quint32 max;
                    const quint8 *buf=buffer.ReceiveStart(max);
                    Q_ASSERT((max%0x2000)==0);
                    Q_ASSERT(buf);
                    packet.Init(buf,pktSize);
                    if (!packet.IsValid())
                        qWarning("Invalid packet!");
                    if (t && !t->valid) {
                        t->valid=true;
                        t->offset=recv;
                        t->timeInt=packet.TimeInteger();
                        t->timeFrac=packet.TimeFraction();
                    }
                } else
                    break;
            }
            const quint32 *sbuf=packet.Samples();
            quint32 scount=packet.SamplesLen();
            if (amt>(scount-partial))
                amt=scount-partial;
            memcpy(&samples[recv],&sbuf[partial],amt*sizeof(quint32));
            partial+=amt;
            if (partial==scount) {
                buffer.ReceiveComplete(pktSize);
                partial=0;
            }
            recv+=amt;
        }
    } else
        recv=buffer.Receive(reinterpret_cast<quint8*>(samples),count*sizeof(quint32)/sizeof(quint32));
    return recv;
}

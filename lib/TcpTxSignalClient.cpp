//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "TcpTxSignalClient.h"

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <QCoreApplication>

bool TcpTxSignalClient::verbose=false;

TcpTxSignalClient::TcpTxSignalClient(quint8 dn,bool useVita49,
                                     const QString &hostname, quint16 port):
    TxSignalClient(),buffer(verbose)
{
    // LOOK HERE!! need to fix to actually use hostname
    this->dn=dn;
    this->useV49=useVita49;
    this->hostname=hostname;
    this->port=port;
    this->sock=-1;
    this->plen=0;
    sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock>=0) {
#if 0
        const char *A="127.0.0.1";
        struct sockaddr_in addr;
        memset(&addr,0,sizeof(addr));
        addr.sin_addr.s_addr=inet_addr(A);
        addr.sin_family=AF_INET;
        addr.sin_port=htons(port);
        if (::connect(sock,reinterpret_cast<struct sockaddr *>(&addr),sizeof(addr)))
            qWarning("Failed to connect to %s:%d - %s",A,port,strerror(errno));
        else {
//            qDebug("Attach port=%d to fd=%d",port,sock);
            buffer.Attach(sock);
            if (verbose)
                qDebug("Connected to %s:%d",A,port);
        }
#else
        struct addrinfo *res;
        struct sockaddr_in addr;
        const char *LOCALHOST="127.0.0.1";
        memset(&addr,0,sizeof(addr));
        addr.sin_port=htons(port);
        addr.sin_addr.s_addr=inet_addr(LOCALHOST);
        addr.sin_family=AF_INET;
        int ecode=getaddrinfo(hostname.toLatin1().constData(),nullptr,nullptr,&res);
        if (ecode) {
            qWarning("Failed to lookup hostname, '%s'.  rval=%d",qPrintable(hostname),ecode);
        } else {
            struct addrinfo *ai;
            for (ai=res;ai;ai=ai->ai_next) {
                if (ai->ai_family==PF_INET) {
                    struct sockaddr_in *sa=reinterpret_cast<struct sockaddr_in *>(ai->ai_addr);
                    addr.sin_family=AF_INET;
                    addr.sin_addr.s_addr=sa->sin_addr.s_addr;
                    break;
                }
            }
        }
        if (::connect(sock,reinterpret_cast<struct sockaddr *>(&addr),sizeof(addr)))
            qWarning("Failed to connect to %s(%s):%d - %s",
                     qPrintable(hostname),inet_ntoa(addr.sin_addr),port,strerror(errno));
        else {
            buffer.Attach(sock);
            if (verbose)
                qDebug("Connected to %s(%s):%d",
                       qPrintable(hostname),inet_ntoa(addr.sin_addr),port);
        }
#endif
    } else
        qWarning("Failed to create socket");
    nextFrameCount=0;
    nextPacketCount=0;
}

TcpTxSignalClient::~TcpTxSignalClient()
{
    if (sock>=0) {
        qDebug("Disconnect: socket=%d",sock);
        buffer.Detach();
        close(sock);
        sock=-1;
        qDebug("Disconnected.");
    }
}

void TcpTxSignalClient::Flush()
{
    if (verbose)
        qDebug("Flush...");
    buffer.Reset();
    plen=0;
}

quint32 TcpTxSignalClient::Send(const quint8 *data,quint32 len)
{
    quint32 sent=0;
    Q_ASSERT(data);
    Q_ASSERT(len>0);
//    qDebug("Send len=%d useV49=%d",len,useV49);
    if (useV49) {
        const quint32 pktSize=0x2000;   // 8KB packets
        while (sent<len) {
            quint32 amt=len-sent;
            if (plen==0) {
                if (buffer.WaitForReadySend(500)) {
                    quint32 max;
                    quint8 *buf=buffer.SendStart(max);
                    if (max<pktSize) {
                        buffer.SendComplete(0); // forces to the next block
                        buf=buffer.SendStart(max);
                        if (max<pktSize) {
                            qWarning("No space in buffer");
                            break;
                        }
                    }
                    if (buf==nullptr) {
                        qDebug("buffer.SendStart returned null");
                        break;
                    }
                    packet.Attach(dn,0,buf,pktSize,nextFrameCount,nextPacketCount);
                } else {
                    //qDebug("timeout...");
                    break;
                }
            }
            quint8 *sbuf=reinterpret_cast<quint8*>(packet.SampleBuffer());
            quint32 slen=packet.SamplesLen()*sizeof(quint32);
            if (amt>(slen-plen))
                amt=slen-plen;
            memcpy(&sbuf[plen],&data[sent],amt);
            plen+=amt;
            if (plen==slen) {
                buffer.SendComplete(pktSize);
                plen=0;
            }
            sent+=amt;
        }
    } else
        sent=buffer.Send(data,len);
//    if (sent>0)
//        qDebug("Sent %d bytes %lld",sent,buffer.Transferred());
    return sent;
}


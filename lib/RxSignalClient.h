//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef RXSIGCLIENT_H
#define RXSIGCLIENT_H

#include <QObject>

class RxSignalClient : public QObject
{
    Q_OBJECT
public:
    explicit RxSignalClient();
    ~RxSignalClient();
    typedef struct {
        bool valid;
        quint32 offset;
        quint32 timeInt;
        quint64 timeFrac;
    } TimeTag;
    virtual bool IsValid() const=0;
    virtual void Flush()=0;
    virtual bool Done() const=0;
    virtual bool IsConnected() const=0;
    virtual bool WaitForReadyReceive(quint32 msTimeout)=0;
    virtual quint32 Receive(quint8 *data, quint32 len)=0;
    virtual quint32 ReceiveSamples(quint32 *samples,quint32 count,TimeTag *t)=0;
signals:
public slots:
protected:
};

#endif // RXSIGCLIENT_H

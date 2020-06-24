//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef TXSIGCLIENT_H
#define TXSIGCLIENT_H

#include <QObject>

class TxSignalClient : public QObject
{
    Q_OBJECT
public:
    explicit TxSignalClient();
    ~TxSignalClient();
    virtual bool IsValid() const=0;
    virtual void Flush()=0;
    virtual bool Done() const=0;
    virtual bool IsConnected() const=0;
//    virtual bool WaitForConnected(int msTimeout)=0;
//    virtual QString ErrorString()=0;
    virtual bool WaitForReadySend(quint32 msTimeout)=0;
    virtual quint32 Send(const quint8 *data, quint32 len)=0;

signals:
public slots:
protected:
};

#endif // TXSIGCLIENT_H

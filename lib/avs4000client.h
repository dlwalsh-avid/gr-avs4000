//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef AVS4000CLIENT_H
#define AVS4000CLIENT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QMutex>
#include <QAtomicInt>

#include "TcpJSONClient.h"

class Script;
class RxSignalClient;
class TxSignalClient;

class AVS4000Client : public QObject
{
    Q_OBJECT
public:
    explicit AVS4000Client(const QString &hostname,quint8 dn);
    ~AVS4000Client();
    static AVS4000Client *Get(const QString &hostname,quint8 dn);

    void Init(Script *script);
    RxSignalClient *rxSig;
    TxSignalClient *txSig;

    quint32 GetMasterSampleRate(quint32 &errorCode,QString &errorDetails);
    bool SetMasterSampleRate(quint32 val,quint32 &errorCode,QString &errorDetails);

    bool StartRxData(bool testPatternEnable,
                     quint32 &errorCode,QString &errorDetails);
    bool StopRxData(quint32 &errorCode,QString &errorDetails);
    bool ConnectRxTcp(const QString &hostname, quint16 port,
                      bool useVita49,
                      quint32 &errorCode, QString &errorDetails);
    bool DisconnectRx(quint32 &errorCode,QString &errorDetails);

    quint32 GetRxSampleRate(quint32 &errorCode,QString &errorDetails);
    bool SetRxSampleRate(quint32 val,quint32 &errorCode,QString &errorDetails);

    bool StartTxData(quint32 &errorCode,QString &errorDetails);
    bool StopTxData(quint32 &errorCode,QString &errorDetails);
    bool ConnectTxTcp(const QString &hostname, quint16 port,
                      bool useVita49,
                      quint32 &errorCode, QString &errorDetails);
    bool DisconnectTx(quint32 &errorCode,QString &errorDetails);

    quint32 GetTxSampleRate(quint32 &errorCode,QString &errorDetails);
    bool SetTxSampleRate(quint32 val,quint32 &errorCode,QString &errorDetails);

    bool SetSampleRates(quint32 rx, quint32 tx, quint32 &errorCode, QString &errorDetails);

    QVariantMap Get(const QString &group,quint32 &errorCode,
                    QString &errorDetails);
    QVariant GetParam(const QString &param,quint32 &errorCode,
                      QString &errorDetails);
    bool Set(const QVariantMap &map,quint32 &errorCode,
             QString &errorDetails);
    bool SetParam(const QString &param,QVariant val,quint32 &errorCode,
                  QString &errorDetails);
    bool UpdateRef(const char *refMode,const char *ppsSel,
                   const char *tbSrc,bool sysSync);
    bool WaitForRefUpdate(quint32 &refCount,quint32 msTimeout=20000);

    bool IsConnected() const;
    quint32 GetUpdateCount();

signals:
public slots:
    QString Hostname() const { return hostname; }
    void StopRx();
    void StopTx();
    void DisconnectRx();
    void DisconnectTx();

protected:
    bool SetTimebase(const char *tbSource);
    bool SetRefMode(const char *refMode);
    bool SetPPS(const char *refMode,const char *ppsSource);
    bool SetSysSync(bool sysSync);
    bool CheckRefLock(const char *refMode);

    QString hostname;
    TcpJSONClient *client;
    double rxFreq,txFreq;
    double ddcFreq,ducFreq;
    double ddcOutGain,ducOutGain;
    QMutex mutex;
    quint8 dn;
};


#endif

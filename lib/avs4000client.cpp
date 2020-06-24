//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "avs4000client.h"

#ifdef AVSPROP
#include "avscommon/CmdMap.h"
#include "avscommon/Script.h"
#include "avscommon/Config.h"
#include "avscommon/SettingMap.h"
#include "avscommon/APIError.h"
#endif

#include "RxSignalClient.h"
#include "TcpRxSignalClient.h"
#include "TcpTxSignalClient.h"

#include <QMutexLocker>
#include <QElapsedTimer>

static const char *pRun="Run";
static const char *pConEnable="ConEnable";
static const char *pConType="ConType";
static const char *pConPort="ConPort";
static const char *pUseV49="UseV49";
static const char *pTestPattern="TestPattern";
static const char *pSampleRate="SampleRate";
static const char *pTimeBase="TimeBase";
static const char *pPPSSel="PPSSel";
static const char *pMode="Mode";
static const char *pSysSync="SysSync";
static const char *gRXDATA="rxdata";
static const char *gRX="rx";
static const char *gTXDATA="txdata";
static const char *gTX="tx";
static const char *gMaster="master";
static const char *gREF="ref";

#define AVSAPI_BASEPORT    12900
#define AVSRX_BASEPORT     12700
#define AVSTX_BASEPORT     12800

AVS4000Client *AVS4000Client::Get(const QString &hostname,quint8 dn)
{
        AVS4000Client *client=new AVS4000Client(hostname,dn);
//        qDebug("created...");
        return client;
}

bool AVS4000Client::IsConnected() const
{
    return (client && client->IsConnected());
}

AVS4000Client::AVS4000Client(const QString &hostname, quint8 dn)
{
//    qDebug("AVS4000Client...");
    this->dn=dn;
    this->hostname=hostname;
    this->rxSig=nullptr;
    this->txSig=nullptr;
    client=new TcpJSONClient(hostname,AVSAPI_BASEPORT+dn);
    Q_ASSERT(client);
    client->WaitForConnected();
}

AVS4000Client::~AVS4000Client()
{
    if (client)
        delete client;
    if (rxSig)
        delete rxSig;
}

#ifdef AVSPROP
void AVS4000Client::Init(Script *script)
{
    SettingMap *cfg=Config::GetGroup("api");
    if (cfg)
        cfg->Set("tjson",true);
    if (client)
        client->Init(script);
    script->GlobalObj()->Add("avs4000",this);
}
#endif

quint32 AVS4000Client::GetMasterSampleRate(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map=Get(gMaster,errorCode,errorDetails);
    if (map.contains(pSampleRate))
        return map.value(pSampleRate).toUInt();
    return 0;
}

bool AVS4000Client::SetMasterSampleRate(quint32 val,quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pSampleRate,val);
    map.insert(gMaster,grp);
    return Set(map,errorCode,errorDetails);
}

bool AVS4000Client::StartRxData(bool testPatternEnable,
                                quint32 &errorCode,QString &errorDetails)
{
    Q_ASSERT(client);
    QVariantMap map,grp;
    grp.insert(pRun,true);
    grp.insert(pTestPattern,testPatternEnable);
    map.insert(gRXDATA,grp);
    return Set(map,errorCode,errorDetails);
}


bool AVS4000Client::StopRxData(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pRun,false);
    grp.insert(pConEnable,false);
    map.insert(gRXDATA,grp);
    return Set(map,errorCode,errorDetails);
}

void AVS4000Client::StopRx()
{
    quint32 ecode;
    QString details;
    StopRxData(ecode,details);
}

bool AVS4000Client::ConnectRxTcp(const QString &hostname,quint16 port,
                                 bool useVita49,
                                 quint32 &errorCode,QString &errorDetails)
{
    if (rxSig) {
        if (DisconnectRx(errorCode,errorDetails)==false) return false;
    }
    if (rxSig==nullptr) {
        QVariantMap map,grp;
        grp.insert(pConEnable,true);
        grp.insert(pConType,"tcp");
        grp.insert(pConPort,port);
        grp.insert(pUseV49,useVita49);
        map.insert(gRXDATA,grp);
        bool rval=Set(map,errorCode,errorDetails);
        if (rval) {
//            qDebug("Connecting TCP RX signal...");
            QThread::msleep(100);
            this->rxSig=new TcpRxSignalClient(useVita49,hostname,port);
    //        QThread::msleep(100);
//            rval=this->rxSig->WaitForConnected(4000);
            rval=this->rxSig->IsConnected();
        }
        return rval;
    }
    errorCode=apiFailure;
    errorDetails.sprintf("Already connected");
    return false;
}

bool AVS4000Client::DisconnectRx(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pConEnable,false);
    map.insert(gRXDATA,grp);
    bool rval=Set(map,errorCode,errorDetails);
    if (rxSig) {
        delete rxSig;
        rxSig=nullptr;
    }
    return rval;
}

void AVS4000Client::DisconnectRx()
{
    quint32 ecode;
    QString details;
    DisconnectRx(ecode,details);
}


quint32 AVS4000Client::GetRxSampleRate(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map=Get(gRX,errorCode,errorDetails);
    if (map.contains(pSampleRate))
        return map.value(pSampleRate).toUInt();
    return 0;
}


bool AVS4000Client::SetRxSampleRate(quint32 val,quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pSampleRate,val);
    map.insert(gRX,grp);
    return Set(map,errorCode,errorDetails);
}

bool AVS4000Client::StartTxData(quint32 &errorCode, QString &errorDetails)
{
    Q_ASSERT(client);
    QVariantMap map,grp;
    grp.insert(pRun,true);
    map.insert(gTXDATA,grp);
    return Set(map,errorCode,errorDetails);
}

bool AVS4000Client::StopTxData(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pRun,false);
    grp.insert(pConEnable,false);
    map.insert(gTXDATA,grp);
    return Set(map,errorCode,errorDetails);
}

void AVS4000Client::StopTx()
{
    quint32 ecode;
    QString details;
    StopTxData(ecode,details);
}

bool AVS4000Client::ConnectTxTcp(const QString &hostname, quint16 port, bool useVita49,
                                 quint32 &errorCode, QString &errorDetails)
{
    if (txSig) {
        if (DisconnectTx(errorCode,errorDetails)==false) return false;
    }
    if (txSig==nullptr) {
        QVariantMap map,grp;
        grp.insert(pConEnable,true);
        grp.insert(pConType,"tcp");
        grp.insert(pConPort,port);
        grp.insert(pUseV49,useVita49);
        grp.insert(pRun,false);
        map.insert(gTXDATA,grp);
        bool rval=Set(map,errorCode,errorDetails);
        if (rval) {
            qDebug("Connecting TCP TX signal...");
            QThread::msleep(100);
            this->txSig=new TcpTxSignalClient(dn,useVita49,hostname,port);
    //        QThread::msleep(100);
            rval=this->txSig->IsConnected();
            qDebug("TCP TX connected=%d",rval);
        }
        return rval;
    }
    errorCode=apiFailure;
    errorDetails.sprintf("Already connected");
    return false;
}

bool AVS4000Client::DisconnectTx(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pConEnable,false);
    grp.insert(pRun,false);
    map.insert(gTXDATA,grp);
    bool rval=Set(map,errorCode,errorDetails);
    if (txSig) {
        delete txSig;
        txSig=nullptr;
    }
    return rval;
}

void AVS4000Client::DisconnectTx()
{
    quint32 ecode;
    QString details;
    DisconnectTx(ecode,details);
}

quint32 AVS4000Client::GetTxSampleRate(quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map=Get(gTX,errorCode,errorDetails);
    if (map.contains(pSampleRate))
        return map.value(pSampleRate).toUInt();
    return 0;
}


bool AVS4000Client::SetTxSampleRate(quint32 val,quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,grp;
    grp.insert(pSampleRate,val);
    map.insert(gTX,grp);
    return Set(map,errorCode,errorDetails);
}


bool AVS4000Client::SetSampleRates(quint32 rx,quint32 tx,
                                   quint32 &errorCode,QString &errorDetails)
{
    QVariantMap map,rxGrp,txGrp;
    rxGrp.insert(pSampleRate,QVariant(rx));
    txGrp.insert(pSampleRate,QVariant(tx));
    map.insert(gRX,rxGrp);
    map.insert(gTX,txGrp);
    return Set(map,errorCode,errorDetails);
}

QVariantMap AVS4000Client::Get(const QString &grp, quint32 &errorCode,
                               QString &errorDetails)
{
    return client->Get(grp,errorCode,errorDetails);
}

QVariant AVS4000Client::GetParam(const QString &param,quint32 &errorCode,
                                 QString &errorDetails)
{
    return client->GetParam(param,errorCode,errorDetails);
}

bool AVS4000Client::Set(const QVariantMap &map,quint32 &errorCode,
                        QString &errorDetails)
{
    return client->Set(map,errorCode,errorDetails);
}

bool AVS4000Client::SetParam(const QString &param, QVariant val, quint32 &errorCode,
                             QString &errorDetails)
{
     return client->SetParam(param,val,errorCode,errorDetails);
}

bool AVS4000Client::SetTimebase(const char *tbSource) {
    if (tbSource) {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gREF,pTimeBase);
        bool rval=client->SetParam(param,QVariant(tbSource),ecode,details);
        if (!rval)
            qWarning("SetTimebase Failed: %s",qPrintable(details));
        return rval;
    }
    return false;
}

bool AVS4000Client::SetRefMode(const char *refMode) {
    if (refMode) {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gREF,pMode);
        bool rval=SetParam(param,QVariant(refMode),ecode,details);
        if (!rval)
            qWarning("SetRefMode Failed: %s",qPrintable(details));
        return rval;
    }
    return false;
}

bool AVS4000Client::SetPPS(const char *refMode,const char *ppsSource) {
    if (refMode) {
        // if GPSDO or PPS modes then we don't set the PPS but return true
        // so the startup process may continue
        if (strcasecmp(refMode,"GPSDO")==0) return true;
        if (strcasecmp(refMode,"PPS")==0) return true;
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gREF,pPPSSel);
        bool rval=SetParam(param,QVariant(ppsSource),ecode,details);
        if (!rval)
            qWarning("SetPPS Failed: %s",qPrintable(details));
        return rval;
    }
    return false;
}

bool AVS4000Client::CheckRefLock(const char *refMode) {
    if (!refMode) return false;
    // pretend we are locked if internal mode
    if (strcasecmp(refMode,"internal")==0) return true;
    if (strcasecmp(refMode,"internalstatic")==0) return true;
    quint32 ecode;
    QString details;
    QVariant lock=GetParam("ref.lock",ecode,details);
    if (lock.isValid()) {
        qDebug("Ref %s",lock.toBool()?"LOCKED":"NOT LOCKED");
        return lock.toBool();
    }
    qWarning("CheckRefLock failed.");
    return false;
}

bool AVS4000Client::SetSysSync(bool sysSync) {
//    if (!sysSync) return true;
    quint32 ecode;
    QString details;
    QString param;
    param.sprintf("%s.%s",gREF,pSysSync);
    bool rval=SetParam(param,QVariant(sysSync),ecode,details);
    if (!rval)
        qWarning("SetSysSync Failed: %s",qPrintable(details));
    return rval;
}

bool AVS4000Client::UpdateRef(const char *refMode,const char *ppsSel,
                              const char *tbSrc,bool sysSync)
{
    QElapsedTimer et;
    et.start();
    if (!SetRefMode(refMode)) return false;
    if (!CheckRefLock(refMode)) return false;
    if (!SetPPS(refMode,ppsSel)) return false;
    if (!SetSysSync(sysSync)) return false;
    if (!SetTimebase(tbSrc)) return false;
    qDebug("UpdateRef: elapsed=%lld ms",et.elapsed());
    return true;
}

quint32 AVS4000Client::GetUpdateCount()
{
    quint32 ecode;
    QString details;
    QVariant count=GetParam("ref.updateCount",ecode,details);
    if (count.isValid())
        return count.toUInt();
    return 0;
}


bool AVS4000Client::WaitForRefUpdate(quint32 &refCount,quint32 msTimeout)
{
    QElapsedTimer et;
    et.start();
    quint32 updateCount=GetUpdateCount();
    for (quint32 i=0;i<msTimeout/100;i++) {
        if (updateCount>refCount) break;
        QThread::msleep(100);
        updateCount=GetUpdateCount();
    }
    qDebug("WaitForRefUpdate: Exit updateCount=%d refCount=%d elapsed=%lld ms",
           updateCount,refCount,et.elapsed());
    if (updateCount>refCount) {
        refCount=updateCount;
        return true;
    }
    return false;
}


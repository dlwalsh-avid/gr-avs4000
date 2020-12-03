//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef TCPJSONCLIENT_H
#define TCPJSONCLIENT_H

#include <QObject>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QMutex>
#include <QTimer>

class QTcpSocket;
#ifdef AVSPROP
class Script;
class SettingMap;
#else
enum APIError {
    apiSuccess=0,
    apiSyntaxError,
    apiInvalidCommand,
    apiMissingCommand,
    apiInvalidParam,
    apiMissingParam,
    apiParamInvalidType,
    apiParamInvalidValue,
    apiParamOutOfRange,
    apiParamReadOnly,
    apiParamWriteOnly,
    apiParamConflict,
    apiParamError,
    apiInvalidConfigGroup,
    apiInvalidConfigParameter,
    apiTimeout,
    apiFailure,
    apiPartialCommit,
    apiAccessDenied,
    apiBusy,
    apiNumErrors
};
#endif

class TcpJSONClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpJSONClient(const QString &hostname,quint16 port);
    ~TcpJSONClient();
    static bool verbose;
    static bool GetVerbose() { return verbose; }
    static void SetVerbose(bool val) { verbose=val; }
    static QString APICall(const QString &hostname, quint16 port,
                           const QString &req, int msTimeout=1000);
#ifdef AVSPROP
    void Init(Script *script);
#endif
    bool Commit(quint32 &ec,QString &estr);
    bool Discard(quint32 &ec,QString &estr);

signals:
    void Initialize();
    void Uninitialize();
    void Connected();
    void Write(const QVariant &req);
    void ResponseAvailable();
    void Busy(const QVariantList &list);

public slots:
    QVariant GetParam(const QString &param,quint32 &errorCode,QString &errorDetails);
    QVariantMap Get(const QString &group,quint32 &errorCode,QString &errorDetails);
    QString GetCmd(const QString &group="");
    QString GetPCmd(const QString &group="");
    QString InfoCmd(const QString &group="");
    bool Set(const QVariantMap &map,quint32 &errorCode,QString &errorDetails);
    bool SetParam(const QString &param,QVariant val,quint32 &errorCode,QString &errorDetails);

    QString SetCmd(const QVariantMap &map);
    QString SetCmd(const QString &param,const QVariant &val);
    bool SetN(const QVariantMap &map,quint32 &errorCode,QString &errorDetails);
    QString SetNCmd(const QVariantMap &map);
    QString CommitCmd();
    QString DiscardCmd();
    QString GetErrCmd();
    QString GetCmdCmd();

    bool WaitForConnected(int msTimeout=10000);
    bool IsConnected() const;
    void SendBusyReq();

protected slots:
    void Init();
    void Uninit();
    void ReadAvailable();
    void Disconnected();
    void Reconnect();
    void WriteREQ(const QVariant &req);
    void BusyREQ(const QVariantList &req);

protected:
    QString DisplayConfig(const QVariantMap &map);
    void SendREQ(const QVariant &req);
    bool WaitForResponse(int msTimeout);
    bool Call(const QString &cmd, const QVariant &arg, QVariant &rval,
              quint32 &errorCode, QString &errorDetails, int msTimeout=15000);
    QVariant rsp;
    QString hostname;
    quint16 port;
    QTcpSocket *socket;
    QThread thread;
    QMutex mutex,busyMutex;
    QVariantList busyReq;
    QTimer *timer;
};

#endif // TCPJSONCLIENT_H

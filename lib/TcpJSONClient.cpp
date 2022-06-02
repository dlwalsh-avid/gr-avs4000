//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "TcpJSONClient.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>

#ifdef AVSPROP
#include "avscommon/Script.h"
#include "avscommon/Debug.h"
#include "avscommon/Config.h"
#include "avscommon/APIError.h"
#endif

bool TcpJSONClient::verbose=false;
//bool TcpJSONClient::verbose=true;

TcpJSONClient::TcpJSONClient(const QString &hostname, quint16 port) :
    QObject(nullptr)
{
//    qDebug("TcpJSONClient...");
    this->hostname=hostname;
    this->port=port;
    socket=nullptr;
    timer=nullptr;
    connect(this,SIGNAL(Initialize()),this,SLOT(Init()));
    connect(this,SIGNAL(Uninitialize()),this,SLOT(Uninit()));
    connect(this,SIGNAL(Write(QVariant)),this,SLOT(WriteREQ(QVariant)));
    connect(this,SIGNAL(Busy(QVariantList)),this,SLOT(BusyREQ(QVariantList)));
    moveToThread(&thread);
    thread.start(QThread::HighPriority);
    emit Initialize();
//    qDebug("TcpJSONClient constructor complete.");
}

TcpJSONClient::~TcpJSONClient()
{
//    qDebug("TcpJSONClient::destructor..");
    emit Uninitialize();
    QThread::msleep(1);
    thread.quit();
    thread.wait(1000);
    disconnect(this,SIGNAL(Initialize()),this,SLOT(Init()));
    disconnect(this,SIGNAL(Uninitialize()),this,SLOT(Uninit()));
    disconnect(this,SIGNAL(Write(QVariant)),this,SLOT(WriteREQ(QVariant)));
}

#ifdef AVSPROP
void TcpJSONClient::Init(Script *script)
{
    Q_ASSERT(script);
//    if (cfg->Get(nsTJSON).toBool()) {
        Debug::AddDebug("tjclient",TcpJSONClient::GetVerbose,
                                   TcpJSONClient::SetVerbose,this);
//        script->GlobalObj()->Add("tjclient",this);
        script->GlobalObj()->Add("api",this);
        CmdMap *cm=new CmdMap("api");
        cm->Add("get","api.GetCmd(%S)","Get API parameters",
                "Usage:\n"
                "      api get                // Display all parameters\n"
                "      api get [group]        // Display parameters for a group\n"
                "      api get [group.param]  // Display a single parameter\n"
                "Exmaples:\n"
                "      api get rx             // Display RX parameter group\n"
                "      api get rx.freq        // Display RX.FREQ parameter\n");
        cm->Add("set","api.SetCmd('%0',%1)","Set API parameters",
                "Usage:\n"
                "      api set <group.param> <val>\n"
                "Exmaples:\n"
                "      api set rx.freq 2.1e9\n"
                "      api set rx.lbmode 'auto'\n");
        cm->Add("getp","api.GetPCmd(%S)","Get pending API parameters",
                "Usage:\n"
                "      api getp               // Display pending parameters\n"
                "      api getp [group]       // Display pending params for a group\n"
                "Exmaple:\n"
                "      api getp rx            // Display RX pending parameters\n");
        cm->Add("setn","api.SetNCmd('%0',%1)","Set API params without commit",
                "Usage:\n"
                "      api setn <group.param> <val>\n");
        cm->Add("commit","api.CommitCmd()","Commit API parameters",
                "Usage:\n"
                "      api commit\n");
        cm->Add("discard","api.DiscardCmd()","Discard pending API parameters",
                "Usage:\n"
                "      api discard\n");
        cm->Add("info","api.InfoCmd(%S)","Display Info about API parameters",
                "Usage:\n"
                "      api info [group]\n");
        cm->Add("geterr","api.GetErrCmd()","Get List of API Errors");
        cm->Add("getcmd","api.GetCmdCmd()","Get List of API Commands");
        cm->Add("rd","api.ReadCmd(%S)","Read Register",
                "Usage:\n"
                "      api rd [group] [reg]   // Read a single register\n"
                "Example:\n"
                "      api rd fpga 0x3025     // Read FPGA Register\n");
        cm->Add("wr","api.WriteCmd('%0','%1',%2)","Write Register",
                "Usage:\n"
                "      api wr <group> <reg> <val>\n"
                "Example:\n"
                "      api wr fpga 0x4003 8   // Write FPGA Register\n");
        script->GlobalCmd()->AddNested("api",cm,"API Parameter related commands");
//    }
}
#endif

void TcpJSONClient::Init()
{
    Uninit();
    if (!timer) {
        timer=new QTimer();
        connect(timer,SIGNAL(timeout()),this,SLOT(SendBusyReq()));
        timer->setSingleShot(true);
    }
//    qDebug("TcpJSONClient::Init...");
//    if (cfg->Get(nsTJSON).toBool()) {
        socket=new QTcpSocket();
        socket->setReadBufferSize(64*1024);
        socket->setSocketOption(QAbstractSocket::LowDelayOption,true);
        socket->connectToHost(hostname,port);
        if (!socket->waitForConnected(5000))
            qWarning("failed to connect to server");
        else {
//            emit Connected();
            qDebug("Connected to %s:%d",qPrintable(hostname),port);
        }
        connect(socket,SIGNAL(readyRead()),this,SLOT(ReadAvailable()));
        connect(socket,SIGNAL(disconnected()),this,SLOT(Disconnected()));
        emit Connected();
//        qDebug("TcpJSONCient: emit connected=%d",IsConnected());
//    } else
//        qWarning("Not enabled.");
}

void TcpJSONClient::Uninit()
{
//    qDebug("TcpJSONClient::Uninit...");
    if (socket) {
        disconnect(socket,SIGNAL(readyRead()),this,SLOT(ReadAvailable()));
        disconnect(socket,SIGNAL(disconnected()),this,SLOT(Disconnected()));
        socket->close();
        delete socket;
        socket=nullptr;
    }
    if (timer) {
        timer->stop();
        disconnect(timer,SIGNAL(timeout()),this,SLOT(SendBusyReq()));
        delete timer;
        timer=nullptr;
    }
}

bool TcpJSONClient::IsConnected() const
{
    if (socket)
        return socket->isValid();
    return false;
}

void TcpJSONClient::Disconnected()
{
    qDebug("disconnected...");
    Reconnect();
}

void TcpJSONClient::Reconnect()
{
    qDebug("Reconnect...");
    emit Initialize();
}

void TcpJSONClient::SendREQ(const QVariant &req)
{
    emit Write(req);
}

void TcpJSONClient::WriteREQ(const QVariant &req)
{
    rsp.clear();    // clear RSP
//    qDebug("rsp.isValid=%d",rsp.isValid());
    QJsonDocument doc=QJsonDocument::fromVariant(req);
    QByteArray bytes=doc.toJson(QJsonDocument::Compact);
    if (verbose)
        qDebug("REQ: %s",qPrintable(QString(bytes)));
    bytes.append("\n");
    int len=bytes.length();
    if (socket==nullptr || !socket->isValid())
        qWarning("Unable to write REQ: invalid socket");
    else {
        while (bytes.length()>0) {
            qint64 rval=socket->write(bytes);
            if (rval<0) {
                qWarning("socket error='%s'",qPrintable(socket->errorString()));
                return;
            }
//            qDebug("rval=%d len=%d",quint32(rval),bytes.length());
            bytes.remove(0,int(rval));
        }
        socket->waitForBytesWritten(1000);
        if (verbose)
            qDebug("Sent %u bytes",len);
    }
}

void TcpJSONClient::ReadAvailable()
{
    rsp.clear();
    if (socket && socket->isValid()) {
        QByteArray bytes=socket->readLine(64*1024);
        // The bytes returned could only be a partial
        // response, so the remaining portion of the data
        // needs to be read.
        while (bytes.indexOf('\n')<0) {
//            qDebug("bytes.len=%d",bytes.length());
            QByteArray B;
            for (int i=0;i<10;i++) {
                // in between reads, we need to do a wait
                // that lets the event loop to run, this is
                // needed for the QTcpSocket to work.
                QEventLoop loop;
                QTimer::singleShot(10,&loop,SLOT(quit()));
                loop.exec();
                B=socket->readLine(64*1024);
//                qDebug("B.len=%d",B.length());
                if (B.length()>0) break;
            }
//            qDebug("B.len=%d",B.length());
            if (B.length()==0) break;
            bytes.append(B);
        }
//        qDebug("bytes.len=%d",bytes.length());
        QJsonParseError error;
        QJsonDocument doc=QJsonDocument::fromJson(bytes,&error);
        if (doc.isNull()) {
            qWarning("Error parsing JSON: %s len=%d\n%s",
                     qPrintable(error.errorString()),bytes.length(),
                     bytes.constData());
//            emit ParseError();
        } else {
            if (verbose)
                qDebug("RSP: %s",qPrintable(QString(doc.toJson(QJsonDocument::Compact))));
            rsp=doc.toVariant();
            emit ResponseAvailable();
        }
    } else
        qWarning("Invalid socket");
}


bool TcpJSONClient::WaitForConnected(int msTimeout)
{
#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed()<qint64(msTimeout)) {
        if (IsConnected()) break;
        QThread::msleep(1);
    }
#else
    QEventLoop loop;
    connect(this,&TcpJSONClient::Connected,&loop,&QEventLoop::quit,Qt::QueuedConnection);
//    connect(this,&TcpJSONClient::Connected,this,&TcpJSONClient::TestSlot,Qt::QueuedConnection);
    QTimer::singleShot(msTimeout,&loop,&QEventLoop::quit);
    qDebug("TcpJSONClient::WaitForConnected: msTimeout=%d",msTimeout);
    qDebug("Loop Level=%d",QThread::currentThread()->loopLevel());
    if (IsConnected()) return true;
    qDebug("loop.exec()");
    loop.exec();
#endif
//    qDebug("isConnected=%d",IsConnected());
    return IsConnected();
}

bool TcpJSONClient::WaitForResponse(int msTimeout)
{
#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed()<qint64(msTimeout)) {
//        mutex.lock();
        bool valid=rsp.isValid();
//        mutex.unlock();
        if (valid) break;
        QThread::msleep(1);
    }
//    qDebug("elapsed %d ms",int(timer.elapsed()));
#else
    QEventLoop loop;
    connect(this,SIGNAL(ResponseAvailable()),&loop,SLOT(quit()));
    QTimer::singleShot(msTimeout,&loop,SLOT(quit()));
    loop.exec();
#endif
    return rsp.isValid();
}

void TcpJSONClient::SendBusyReq()
{
    QMutexLocker locker(&busyMutex);
    if (busyReq.length()>0) {
//        qDebug("SendBusyReq");
        QString cmd=busyReq[0].toString();
        QVariant arg=(busyReq.length()>1)?busyReq[1]:QVariant();
        quint32 ecode;
        QString details;
        QVariant rval;
        busyReq.clear();
        locker.unlock();
        if (!Call(cmd,arg,rval,ecode,details) && ecode!=apiBusy)
            qWarning("Busy Req failed: %s",qPrintable(details));
    }
}

void TcpJSONClient::BusyREQ(const QVariantList &req)
{
    QMutexLocker locker(&busyMutex);
    busyReq=req;
    Q_ASSERT(timer);
    timer->start(5);
}


bool TcpJSONClient::Call(const QString &cmd, const QVariant &arg, QVariant &rval,
                           quint32 &errorCode, QString &errorDetails, int msTimeout)
{
//    qDebug("Call");
    bool success=false;
    QVariantList req;
    req.append(QVariant(cmd));
    if (arg.isValid() && !arg.isNull())
        req.append(arg);
    if (!mutex.tryLock(0)) {
        // Dont send the request but instead queue it as a Busy REQ
        // This will start a timer.  When the timer expires the request will be
        // sent.
//        qDebug("Busy!");
        emit Busy(req);
        return true;
    }
    busyMutex.lock();
    busyReq.clear();    // clears any pending busy request
    busyMutex.unlock();
    rsp.clear();    // SendREQ is implemented via Signal/Slot which can cause RSP to
                    // NOT be cleared before WaitForResponse is called, this will
                    // cause WaitForResponse to return immediately.
    SendREQ(req);
    rval.clear();
    if (WaitForResponse(msTimeout)) {
//        qDebug("Got RSP");
        if (rsp.type()==QVariant::Bool)
            return rsp.toBool();
        QVariantList list=rsp.toList();
        if (list.length()>0)
            success=list[0].toBool();
        if (success && list.length()>1)
            rval=list[1];
        if (!success) {
            if (list.length()<3)
                qWarning("RSP failed but does not contain error information");
            else {
                errorCode=list[1].toUInt();
                errorDetails=list[2].toString();
            }
        }
    } else {
        errorCode=apiTimeout;
        errorDetails="No Response";
    }
//    qDebug("CMD: %s success=%d",qPrintable(cmd),success);
    mutex.unlock();
    return success;
}

QVariantMap TcpJSONClient::Get(const QString &group, quint32 &errorCode, QString &errorDetails)
{
//    QString out;
    QVariant rval;
    if (Call("GET",QVariant(group),rval,errorCode,errorDetails)) {
        QVariantMap map=rval.toMap();
        if (map.contains(group))
            return map.value(group).toMap();
    }
    return QVariantMap();
}

QVariant TcpJSONClient::GetParam(const QString &param,quint32 &errorCode,QString &errorDetails)
{
    QVariant rval;
    if (Call("GET",QVariant(param),rval,errorCode,errorDetails))
        return rval;
    return QVariantMap();
}

QString TcpJSONClient::GetCmd(const QString &group)
{
    QString out;
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("GET",QVariant(group),rval,ec,estr)) {
        if (rval.type()==QVariant::Map)
            out=DisplayConfig(rval.toMap());
        else
            out=rval.toString();
    } else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

QString TcpJSONClient::GetPCmd(const QString &group)
{
    QString out;
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("GETP",QVariant(group),rval,ec,estr))
        out=DisplayConfig(rval.toMap());
    else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

QString TcpJSONClient::InfoCmd(const QString &group)
{
    QString out;
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("INFO",QVariant(group),rval,ec,estr))
        out=DisplayConfig(rval.toMap());
    else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

bool TcpJSONClient::Set(const QVariantMap &map, quint32 &errorCode, QString &errorDetails)
{
    QVariant rval;
    return Call("SET",map,rval,errorCode,errorDetails);
}

bool TcpJSONClient::SetParam(const QString &param,QVariant val,quint32 &errorCode, QString &errorDetails)
{
    QStringList list=param.split(".");
    // param name must be in GRP.PARAM format
    if (list.count()!=2) return false;
    QString g=list[0];
    QString p=list[1];
    if (g.length()==0 || p.length()==0) return false;
    QVariantMap map,grp;
    grp.insert(p,val);
    map.insert(g,grp);
    QVariant rval;
    return Call("SET",map,rval,errorCode,errorDetails);
}

QString TcpJSONClient::SetCmd(const QString &param,const QVariant &val)
{
    if (param.contains('.')) {
        QStringList list=param.split('.');
        QString g=list[0];
        QString p=list[1];
        QVariantMap grp;
        QVariantMap map;
        grp.insert(p,val);
        map.insert(g,grp);
        return SetCmd(map);
    }
    return "Invalid group.parameter name";
}


QString TcpJSONClient::SetCmd(const QVariantMap &map)
{
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("SET",map,rval,ec,estr)) {
        return "Set";
    }
    return estr;
}

bool TcpJSONClient::SetN(const QVariantMap &map, quint32 &errorCode, QString &errorDetails)
{
    QVariant rval;
    return Call("SETN",map,rval,errorCode,errorDetails);
}

QString TcpJSONClient::SetNCmd(const QVariantMap &map)
{
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("SETN",map,rval,ec,estr)) {
        return "Set";
    }
    return estr;
}

bool TcpJSONClient::Commit(quint32 &ec,QString &estr)
{
    QVariant rval;
    return Call("COMMIT",QVariant(),rval,ec,estr);
}


QString TcpJSONClient::CommitCmd()
{
    QString out;
    quint32 ec;
    QString estr;
    if (Commit(ec,estr))
        out="Committed";
    else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

bool TcpJSONClient::Discard(quint32 &ec,QString &estr)
{
    QVariant rval;
    return Call("DISCARD",QVariant(),rval,ec,estr);
}


QString TcpJSONClient::DiscardCmd()
{
    QString out;
    quint32 ec;
    QString estr;
    if (Discard(ec,estr))
        out="Discarded";
    else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

QString TcpJSONClient::ValToString(QVariant val) const
{
    QString str;
//    qDebug("type=%s",val.typeName());
    if (val.type()==QVariant::Bool)
        str=val.toBool()?"true":"false";
    // Unfortunately the following doesn't work.
    // All numbers seem to come out of the JSON Parser
    // as doubles. -dlw 12/15/20
    else if (val.type()==QVariant::UInt || val.type()==QVariant::Int)
        str.sprintf("0x%02X (%d)",val.toUInt(),val.toUInt());
//    else if (val.type()==QVariant::Int)
//        str.sprintf("%d",val.toInt());
    else if (val.type()==QVariant::String)
        str=val.toString();
    else if (val.type()==QVariant::List) {
        QVariantList list=val.toList();
        str="[";
        for (int i=0;i<list.count();i++) {
            if (i>0) str+=",";
            str+=ValToString(list.at(i));
        }
        str+="]";
    } else //if (val.type()==QVariant::Double)
        str=val.toString();
    // might need to extend for lists or maps

    return str;
}

QString TcpJSONClient::DisplayConfig(const QVariantMap &map)
{
    QString out;
    QString group;
    foreach(group,map.keys()) {
        QString line;
        QVariantMap params=map.value(group).toMap();
        QString param;
        foreach(param,params.keys()) {
            QString name;
            name.sprintf("%s.%s",qPrintable(group),qPrintable(param));
            QVariant val=params.value(param);
            QString str=ValToString(val);
            line.sprintf("%25s = %s\n",
                         qPrintable(name),qPrintable(str));
            out+=line;
        }
    }
    return out;
}

QString TcpJSONClient::GetErrCmd()
{
    QString out;
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("GETERR",QVariant(),rval,ec,estr)) {
        out.sprintf("Error Info:\n");
        QVariantList list=rval.toList();
        QVariant var;
        foreach (var,list) {
            QVariantList err=var.toList();
            QString line;
            line.sprintf("    %2d: %s\n",
                         err[0].toInt(),
                         qPrintable(err[1].toString()));
            out+=line;
        }
    } else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

QString TcpJSONClient::GetCmdCmd()
{
    QString out;
    quint32 ec;
    QString estr;
    QVariant rval;
    if (Call("GETCMD",QVariant(),rval,ec,estr)) {
        out.sprintf("Available Commands:\n");
        QVariantList list=rval.toList();
        QVariant var;
        foreach (var,list) {
            QVariantList c=var.toList();
            QString line;
            line.sprintf("    %-12s%s\n",
                         qPrintable(c[0].toString()),
                         qPrintable(c[1].toString()));
            out+=line;
        }
    } else
        out.sprintf("Error %u: %s",ec,qPrintable(estr));
    return out;
}

QString TcpJSONClient::APICall(const QString &hostname,
                               quint16 port,
                               const QString &req,
                               int msTimeout)
{
    QString out;
    QTcpSocket sock;
    sock.connectToHost(hostname,port);
    qDebug("%s:%d <- %s",qPrintable(hostname),port,qPrintable(req));
    QString rstr=req+"\n";
    qint64 rval=sock.write(rstr.toLatin1());
    sock.waitForBytesWritten(500);
    if (rval==rstr.length()) {
        sock.waitForReadyRead(msTimeout);
        return QString(sock.readAll());
    }
    out.sprintf("Failed to send");
    return out;
}

QString TcpJSONClient::ReadCmd(const QString &grp,const QVariant &arg)
{
    quint32 ec;
    QString estr;
    QVariantList list;
    list.append(grp);
    list.append(arg);
    QVariant rval;
    if (Call("RD",list,rval,ec,estr))
        return ValToString(rval);
    return estr;
}

QString TcpJSONClient::WriteCmd(const QString &grp,const QVariant &arg,const QVariant &val)
{
    quint32 ec;
    QString estr;
    QVariantList list;
    list.append(grp);
    list.append(arg);
    list.append(val);
    QVariant rval;
    if (Call("WR",list,rval,ec,estr))
        return "Written.";
    return estr;
}

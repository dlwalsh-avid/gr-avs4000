/* -*- c++ -*- */
/* 
 * Copyright 2020 Avid Systems, Inc.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if 1
#include <gnuradio/io_signature.h>
#include <gnuradio/logger.h>
#include "TcpTxSignalClient.h"
#include "avs4000client.h"
#include <stdio.h>
#include "avs4000tx_impl.h"

//#define AVSAPI_PORT     12904
#define AVSRX_BASEPORT     12700
#define AVSTX_BASEPORT     12800

#define BUFFER_SAMPLES  256*1024
//static qint16 tBuf[BUFFER_SAMPLES*2];

static const char *pRun="Run";
static const char *pConEnable="ConEnable";
static const char *pConType="ConType";
static const char *pConPort="ConPort";
static const char *pUseV49="UseV49";
static const char *pTestPattern="TestPattern";
static const char *pLoopback="loopback";
static const char *pSampleRate="SampleRate";
static const char *pFreq="Freq";
static const char *pOutGain="OutGain";
static const char *pTimeBase="TimeBase";
static const char *pPPSSel="PPSSel";
static const char *pMode="Mode";
static const char *pSysSync="SysSync";
static const char *pAmpEnable="AmpEnable";
static const char *pOutRxEnable="OutRxEnable";
static const char *pRFBW="RFBW";
static const char *pStartMode="StartMode";
static const char *gTXDATA="txdata";
static const char *gTX="tx";
static const char *gMaster="master";
static const char *gDUC="duc";
static const char *gREF="ref";

namespace gr {
  namespace avs4000 {

    avs4000tx::sptr
    avs4000tx::make(const std::string &host,int dn,
                    double rate,double txFreq,double txRFBW,
                    double ducFreq,double ducOutGain,
                    bool ampEnable,bool outRxEn,
                    const char *startMode,bool refMaster,
                    const char *tbSource,const char *refMode,
                    const char *ppsSource,bool sysSync)
    {
      return gnuradio::get_initial_sptr
        (new avs4000tx_impl(host,dn,
                            rate,txFreq,txRFBW,
                            ducFreq,ducOutGain,
                            ampEnable,outRxEn,
                            startMode,refMaster,
                            tbSource,refMode,ppsSource,sysSync));
    }

    /*
     * The private constructor
     */
    avs4000tx_impl::avs4000tx_impl(const std::string &host,int dn,
                                   double rate, double txFreq, double txRFBW,
                                   double ducFreq,double ducOutGain,
                                   bool ampEnable, bool outRxEn,
                                   const char *startMode, bool refMaster,
                                   const char *tbSource, const char *refMode,
                                   const char *ppsSource, bool sysSync)
      : gr::sync_block("avs4000tx",
                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                       gr::io_signature::make(0, 0, 0))
    {
        client=nullptr;
        tBuf=new qint16[BUFFER_SAMPLES*2];
        this->host=QString(host.c_str());
	this->dn=dn;
        this->rate=rate;
        this->txFreq=txFreq;
        this->txRFBW=txRFBW;
        this->ampEnable=ampEnable;
        this->outRxEn=outRxEn;
        this->ducFreq=ducFreq;
        this->ducOutGain=ducOutGain;
        this->startMode=startMode;
        this->refMaster=refMaster;
        this->tbSource=tbSource;
        this->refMode=refMode;
        this->ppsSource=ppsSource;
        this->sysSync=sysSync;
        this->updateCount=0;
        this->txRetry=0;
    }

    /*
     * Our virtual destructor.
     */
    avs4000tx_impl::~avs4000tx_impl()
    {
        if (tBuf)
            delete tBuf;
    }

    int
    avs4000tx_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        size_t samples=(noutput_items>BUFFER_SAMPLES)?BUFFER_SAMPLES:noutput_items;
        const size_t bytesPerSample=sizeof(qint16)*2;
        size_t bytes=samples*bytesPerSample;
        if (client==nullptr || client->txSig==nullptr) return -1;

        for (int i=0;i<samples;i++) {
//            tBuf[i*2]=qint16(in[i].real());
//            tBuf[i*2+1]=qint16(in[i].imag());
            // The above code was backwards.
            // The Q word comes before the I word. -dlw 5/27/22
            tBuf[i*2]=qint16(in[i].imag());
            tBuf[i*2+1]=qint16(in[i].real());
        }
        // this function needs to not block!!  or only block for short time
        quint32 rval=client->txSig->Send((const quint8*)tBuf,bytes);
        if (rval<bytes) {
//            qWarning("Only sent %d out %ld bytes",rval,bytes);
            if (rval==0 && txRetry++>=20) {
                qWarning("Only sent %d out %ld bytes after %d retries",
                         rval,bytes,txRetry);
                return -1;
            }
        } else
            txRetry=0;
      // Tell runtime system how many output items we produced.
      return samples;
    }

    bool avs4000tx_impl::SetRate(double rate)
    {
        Q_ASSERT(client);
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gTX,pSampleRate);
        bool rval=client->SetParam(param,QVariant(rate),ecode,details);
        if (!rval)
            qWarning("SetRate Failed: %s",qPrintable(details));
        else
            this->rate=rate;
        return rval;
    }

    void avs4000tx_impl::SetTxFreq(double freq)
    {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gTX,pFreq);
        bool rval=client->SetParam(param,QVariant(freq),ecode,details);
        if (!rval && ecode!=apiBusy)
            qWarning("SetTxFreq Failed: %s",qPrintable(details));
        else
            this->txFreq=freq;
    }

    void avs4000tx_impl::SetDUCFreq(double freq)
    {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gDUC,pFreq);
        bool rval=client->SetParam(param,QVariant(freq),ecode,details);
        if (!rval)
            qWarning("SetDUCFreq Failed: %s",qPrintable(details));
        else
            this->ducFreq=freq;
    }

    void avs4000tx_impl::SetDUCOutGain(double val)
    {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gDUC,pOutGain);
        bool rval=client->SetParam(param,QVariant(val),ecode,details);
        if (!rval)
            qWarning("SetDUCOutGain Failed: %s",qPrintable(details));
        else
            this->ducOutGain=val;
    }


    bool avs4000tx_impl::start()
    {
        qDebug("TX start: dn=%d",dn);
        this->client=AVS4000Client::Get(host,dn);
        Q_ASSERT(this->client);
        client->StopTx();
        if (refMaster) {
            if (!client->UpdateRef(refMode,ppsSource,tbSource,sysSync))
                qWarning("Failed to update Ref params");
        } else {
            this->updateCount=client->GetUpdateCount();
            if (!client->WaitForRefUpdate(this->updateCount))
                qWarning("Failed to wait for Ref Update");
        }

        QVariantMap map;
        QVariantMap tx;
        tx.insert(pSampleRate,rate);
        tx.insert(pFreq,txFreq);
        tx.insert(pStartMode,startMode);
        tx.insert(pAmpEnable,ampEnable);
        tx.insert(pOutRxEnable,outRxEn);
        tx.insert(pRFBW,txRFBW);
        map.insert(gTX,tx);
        QVariantMap duc;
        duc.insert(pFreq,ducFreq);
        duc.insert(pOutGain,ducOutGain);
        map.insert(gDUC,duc);
        quint32 ecode;
        QString details;
        bool rval=client->Set(map,ecode,details);
        if (rval) {
            rval=client->ConnectTxTcp(host,AVSTX_BASEPORT+dn,true,ecode,details) &&
                 client->StartTxData(ecode,details);
        } else
            qDebug("Tx Set failed.");
        quint32 delay=(100*dn+50)%1000;
        qDebug("delay=%d ms",delay);
        if (rval)
            usleep(delay*1000); // delay
        if (!rval)
            qWarning("Tx Startup Failed: %s",qPrintable(details));
        return rval;
    }

    bool avs4000tx_impl::stop()
    {
        qDebug("TX Stop: dn=%d",dn);
        if (client) {
            quint32 ecode;
            QString details;
            bool rval=client->StopTxData(ecode,details) &&
                      client->DisconnectTx(ecode,details);
            client->deleteLater();
//            AVS4000Client::Close();
            client=nullptr;
            if (!rval) {
                qWarning("Stop TX Failed: %s",qPrintable(details));
                return false;
            }
        }
        return true;
    }

  } /* namespace avs4000 */
} /* namespace gr */

#endif

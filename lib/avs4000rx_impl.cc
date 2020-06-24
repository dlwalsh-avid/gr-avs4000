/* -*- c++ -*- */
/* 
 * Copyright 2020 <+YOU OR YOUR COMPANY+>.
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
#include <gnuradio/io_signature.h>
#include <gnuradio/logger.h>
#include "TcpRxSignalClient.h"
#include "avs4000client.h"
#include <stdio.h>
#include "avs4000rx_impl.h"

//#define AVSAPI_PORT     12904
#define AVSRX_BASEPORT     12700
#define AVSTX_BASEPORT     12800

#define BUFFER_SAMPLES  256*1024
//static qint16 rBuf[BUFFER_SAMPLES*2];

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
static const char *pRFBW="RFBW";
static const char *pStartMode="StartMode";
static const char *gRXDATA="rxdata";
static const char *gRX="rx";
static const char *gTXDATA="txdata";
static const char *gDDC="ddc";
static const char *gREF="ref";

namespace gr {
  namespace avs4000 {

    avs4000rx::sptr
    avs4000rx::make(int dn,double rate,double rxFreq,double rxRFBW,
                    double ddcFreq,double ddcOutGain,
                    const char *startMode,bool refMaster,
                    const char *tbSource,const char *refMode,
                    const char *ppsSource,bool sysSync)
    {
      return gnuradio::get_initial_sptr
        (new avs4000rx_impl(dn,rate,rxFreq,rxRFBW,
                            ddcFreq,ddcOutGain,
                            startMode,refMaster,
                            tbSource,refMode,ppsSource,sysSync));
    }

    /*
     * The private constructor
     */
    avs4000rx_impl::avs4000rx_impl(int dn,double rate, double rxFreq, double rxRFBW,
                                   double ddcFreq, double ddcOutGain,
                                   const char *startMode,
                                   bool refMaster,
                                   const char *tbSource,
                                   const char *refMode,
                                   const char *ppsSource,
                                   bool sysSync)
      : gr::sync_block("avs4000rx",
              gr::io_signature::make(0,0,0),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
        rBuf=new qint16[BUFFER_SAMPLES*2];
        this->id=pmt::string_to_symbol("avs4000rx");
        client=nullptr;
	this->dn=dn;
        this->startMode=startMode;
        this->refMaster=refMaster;
        this->rate=rate;
        this->rxFreq=rxFreq;
        this->rxRFBW=rxRFBW;
        this->ddcFreq=ddcFreq;
        this->ddcOutGain=ddcOutGain;
        this->tbSource=tbSource;
        this->refMode=refMode;
        this->ppsSource=ppsSource;
        this->sysSync=sysSync;
        this->updateCount=0;
    }

    /*
     * Our virtual destructor.
     */
    avs4000rx_impl::~avs4000rx_impl()
    {
        if (rBuf)
            delete rBuf;
    }

    int
    avs4000rx_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr_complex *out = (gr_complex *) output_items[0];
        quint32 samples=(noutput_items>BUFFER_SAMPLES)?BUFFER_SAMPLES:noutput_items;
        TcpRxSignalClient::TimeTag t;
        static quint32 lastSec=0;
        if (client==nullptr || client->rxSig==nullptr) return -1;
        quint32 rval=client->rxSig->ReceiveSamples(reinterpret_cast<quint32*>(rBuf),
                                                   samples,&t);
        if (rval!=samples) {
//            qWarning("Only recv %d out %d samples",rval,samples);
            return -1;
        }
        if (t.valid) {
            const pmt::pmt_t val=pmt::make_tuple(pmt::from_long(long(t.timeInt)),
                                                 pmt::from_uint64(t.timeFrac));
            add_item_tag(0,                                // Port Number
                         nitems_written(0)+t.offset,       // Sample offset
                         pmt::string_to_symbol("rx_time"), // Key
                         val,id);                          // Value
            if (t.timeInt!=lastSec) {
                lastSec=t.timeInt;
//                qDebug("tag %d",lastSec);
            }
        }
        for (int i=0;i<samples;i++) {
            out[i].real(float(rBuf[i*2]));
            out[i].imag(float(rBuf[i*2+1]));
        }
        return samples;
    }

    bool avs4000rx_impl::SetRate(double rate)
    {
        Q_ASSERT(client);
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gRX,pSampleRate);
        bool rval=client->SetParam(param,QVariant(rate),ecode,details);
        if (!rval)
            qWarning("SetRate Failed: %s",qPrintable(details));
        else
            this->rate=rate;
        return rval;
    }

    void avs4000rx_impl::SetRxFreq(double freq)
    {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gRX,pFreq);
        bool rval=client->SetParam(param,QVariant(freq),ecode,details);
        if (!rval && ecode!=apiBusy)
            qWarning("SetRxFreq Failed: %s",qPrintable(details));
        else
            this->rxFreq=freq;
    }

    void avs4000rx_impl::SetDDCFreq(double freq)
    {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gDDC,pFreq);
        bool rval=client->SetParam(param,QVariant(freq),ecode,details);
        if (!rval)
            qWarning("SetDDCFreq Failed: %s",qPrintable(details));
        else
            this->ddcFreq=freq;
    }

    void avs4000rx_impl::SetDDCOutGain(double val)
    {
        quint32 ecode;
        QString details;
        QString param;
        param.sprintf("%s.%s",gDDC,pOutGain);
        bool rval=client->SetParam(param,QVariant(val),ecode,details);
        if (!rval)
            qWarning("SetDDCOutGain Failed: %s",qPrintable(details));
        else
            this->ddcOutGain=val;
    }

    bool avs4000rx_impl::start()
    {
        qDebug("RX start...");
        this->client=AVS4000Client::Get("localhost",dn);
        Q_ASSERT(this->client);
        client->StopRx();
        if (refMaster) {
            if (!client->UpdateRef(refMode,ppsSource,tbSource,sysSync))
                qWarning("Failed to update Ref params");
        } else {
            this->updateCount=client->GetUpdateCount();
            if (!client->WaitForRefUpdate(this->updateCount))
                qWarning("Failed to wait for Ref Update");
        }
        QVariantMap map;
        QVariantMap rx;
        rx.insert(pSampleRate,rate);
        rx.insert(pFreq,rxFreq);
        rx.insert(pStartMode,startMode);
        rx.insert(pRFBW,rxRFBW);
        map.insert(gRX,rx);
        QVariantMap ddc;
        ddc.insert(pFreq,ddcFreq);
        ddc.insert(pOutGain,ddcOutGain);
        map.insert(gDDC,ddc);
        quint32 ecode;
        QString details;
        bool rval=client->Set(map,ecode,details);
        if (rval) {
            rval=client->ConnectRxTcp("localhost",AVSRX_BASEPORT+dn,true,ecode,details) &&
                 client->StartRxData(false,ecode,details);
        }
        if (!rval)
            qWarning("Rx Startup Failed: %s",qPrintable(details));
        return rval;
    }

    bool avs4000rx_impl::stop()
    {
//        qDebug("stop...");
        if (client) {
            quint32 ecode;
            QString details;
            bool rval=client->StopRxData(ecode,details) &&
                      client->DisconnectRx(ecode,details);
            client->deleteLater();
            client=nullptr;
            if (!rval) {
                qWarning("Stop RX Failed: %s",qPrintable(details));
                return false;
            }
//            AVS4000Client::Close();
        }
        return true;
    }

  } /* namespace avs4000 */
} /* namespace gr */


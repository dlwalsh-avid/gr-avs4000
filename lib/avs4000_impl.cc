/* -*- c++ -*- */
/* 
 * Copyright 2019 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either   3, or (at your option)
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
#include "avs4000_impl.h"
#include "TcpRxSignalClient.h"
#include "TcpTxSignalClient.h"
#include "avs4000client.h"
#include <stdio.h>

#define AVSAPI_PORT     12904
#define AVSRX_PORT     12914
#define AVSTX_PORT     12924

//#define SIMDATA

#define BUFFER_SAMPLES  256*1024
static qint16 rxBuf[BUFFER_SAMPLES*2];
static qint16 txBuf[BUFFER_SAMPLES*2];

namespace gr {
  namespace avs4000 {

    avs4000::sptr
    avs4000::make(double rate,double rxFreq,double txFreq)
    {
      return gnuradio::get_initial_sptr
        (new avs4000_impl(rate,rxFreq,txFreq));
    }

    /*
     * The private constructor
     */
    avs4000_impl::avs4000_impl(double rate, double rxFreq,double txFreq)
      : gr::sync_block("avs4000",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)))
    {
        this->id=pmt::string_to_symbol("avs4000");
        client=NULL;
        this->rate=rate;
        this->rxFreq=rxFreq;
        this->txFreq=txFreq;
    }

    /*
     * Our virtual destructor.
     */
    avs4000_impl::~avs4000_impl()
    {
        // this doesn't seem to be getting called
        qDebug("destructor called...\r\n");
//        if (rx)
//            delete rx;
    }

    int
    avs4000_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
//        qDebug("work...");
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];
      quint32 samples=(noutput_items>BUFFER_SAMPLES)?BUFFER_SAMPLES:noutput_items;
      const size_t bytesPerSample=sizeof(qint16)*2;
      TcpRxSignalClient::TimeTag t;
      static quint32 lastSec=0;
      quint32 rval=client->rxSig->ReceiveSamples(reinterpret_cast<quint32*>(rxBuf),
                                                 samples,&t);
      if (t.valid) {
          const pmt::pmt_t val=pmt::make_tuple(pmt::from_long(long(t.timeInt)),
                                               pmt::from_uint64(t.timeFrac));
          add_item_tag(0,                                // Port Number
                       nitems_written(0)+t.offset,       // Sample offset
                       pmt::string_to_symbol("rx_time"), // Key
                       val,id);                          // Value
          if (t.timeInt!=lastSec) {
              lastSec=t.timeInt;
              qDebug("tag %d",lastSec);
          }
      }
      for (int i=0;i<samples;i++) {
          out[i].real(float(rxBuf[i*2]));
          out[i].imag(float(rxBuf[i*2+1]));
          txBuf[i*2]=qint16(in[i].real());
          txBuf[i*2+1]=qint16(in[i].imag());
      }
      // this function needs to not block!!  or only block for short time
      client->txSig->Send((const quint8*)txBuf,samples*bytesPerSample);
//      qDebug("samples=%d",samples);
      return samples;
    }

    bool avs4000_impl::Startup()
    {
        qDebug("Startup...");
        for (int i=0;i<2;i++) {
            if (client) delete client;
            this->client=new AVS4000Client("localhost",AVSAPI_PORT);
//            if (!cmd->Invoke("adt stop")) continue;
//            QThread::msleep(500);
//            if (!cmd->Invoke("adt set patgen false")) continue;
            if (!SetRate(rate)) continue;
            if (!SetRxFreq(rxFreq)) continue;
            if (!SetTxFreq(txFreq)) continue;
            quint32 ecode;
            QString details;
            bool rval=client->ConnectRxTcp("localhost",AVSRX_PORT,true,false,
                                           ecode,details) &&
                      client->ConnectTxTcp("localhost",AVSTX_PORT,true,false,
                                           ecode,details);
            if (!rval) {
                qWarning("Startup Failed. %s",qPrintable(details));
                return false;
            }
            QThread::msleep(50);
            rval=client->StartTxData(false,ecode,details) &&
                 client->StartRxData(false,false,ecode,details);
            if (!rval)
                qWarning("Startup Failed. %s",qPrintable(details));
            else
                qDebug("suceeded.");
            return rval;
        }
        qDebug("failed.");
        return false;
    }

    bool avs4000_impl::SetRate(double rate)
    {
        Q_ASSERT(client);
        this->rate=rate;
        quint32 ecode;
        QString details;
        bool rval=client->SetSampleRates(quint32(rate),quint32(rate),ecode,details);
        if (!rval)
            qWarning("SetRate Failed: %s",qPrintable(details));
        return rval;
    }

    bool avs4000_impl::SetRxFreq(double freq)
    {
        this->rxFreq=freq;
        quint32 ecode;
        QString details;
        bool rval=client->SetRxFreq(quint64(freq),ecode,details);
        if (!rval)
            qWarning("SetTxFreq Failed: %s",qPrintable(details));
        return rval;
    }
    bool avs4000_impl::SetTxFreq(double freq)
    {
        this->txFreq=freq;
        quint32 ecode;
        QString details;
        bool rval=client->SetTxFreq(quint64(freq),ecode,details);
        if (!rval)
            qWarning("SetTxFreq Failed: %s",qPrintable(details));
        return rval;
    }

    bool avs4000_impl::start()
    {
        qDebug("start...");
        return Startup();
    }

    bool avs4000_impl::stop()
    {
        qDebug("stop...");
        if (client) {
            quint32 ecode;
            QString details;
            bool rval=client->StopRxData(ecode,details) &&
                      client->StopTxData(ecode,details) &&
                      client->DisconnectRx(ecode,details) &&
                      client->DisconnectTx(ecode,details);
            if (!rval)
                qWarning("Stop Error: %s",qPrintable(details));
            delete client;
            client=nullptr;
        }
        return true;
    }
  } /* namespace avs4000 */
} /* namespace gr */


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

#ifndef INCLUDED_AVS4000_AVS4000TX_IMPL_H
#define INCLUDED_AVS4000_AVS4000TX_IMPL_H

#include <avs4000/avs4000tx.h>
#include <QtGlobal>

class AVS4000Client;

namespace gr {
  namespace avs4000 {

    class avs4000tx_impl : public avs4000tx
    {
     private:
      // Nothing to declare in this block.
        AVS4000Client *client;
	int dn;
        double rate,rxFreq,txFreq,ducFreq,ducOutGain,txRFBW;
        const char *startMode;
        const char *tbSource;
        const char *refMode;
        const char *ppsSource;
        bool sysSync,ampEnable,outRxEn,refMaster;
        unsigned int updateCount;
        unsigned int txRetry;
        qint16 *tBuf;

    public:
      avs4000tx_impl(int dn,double rate,double txFreq,double txRFBW,
                     double ducFreq,double ducOutGain,
                     bool ampEnable,bool outRxEn,
                     const char *startMode,bool refMaster,
                     const char *tbSource,const char *refMode,
                     const char *ppsSource,bool sysSync);
      ~avs4000tx_impl();

      bool SetRate(double rate);
      virtual void SetTxFreq(double freq);
      virtual void SetDUCFreq(double freq);
      virtual void SetDUCOutGain(double val);

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
      virtual bool start();
      virtual bool stop();
    };
  } // namespace avs4000
} // namespace gr

#endif /* INCLUDED_AVS4000_AVS4000TX_IMPL_H */


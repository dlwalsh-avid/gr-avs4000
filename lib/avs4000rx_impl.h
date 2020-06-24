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

#ifndef INCLUDED_AVS4000_AVS4000RX_IMPL_H
#define INCLUDED_AVS4000_AVS4000RX_IMPL_H

#include <avs4000/avs4000rx.h>
#include <pmt/pmt.h>

class AVS4000Client;

namespace gr {
  namespace avs4000 {

    class avs4000rx_impl : public avs4000rx
    {
     private:
        AVS4000Client *client;
	int dn;
        double rate,rxFreq,ddcFreq,ddcOutGain,rxRFBW;
        const char *startMode;
        const char *tbSource;
        const char *refMode;
        const char *ppsSource;
        bool sysSync;
        bool refMaster;
        pmt::pmt_t id;
        unsigned int updateCount;
        qint16 *rBuf;

     public:
      avs4000rx_impl(int dn,double rate, double rxFreq,double rxRFBW,
                     double ddcFreq,double ddcOutGain,
                     const char *startMode, bool refMaster,
                     const char *tbSource,
                     const char *refMode,
                     const char *ppsSource,
                     bool sysSync);
      ~avs4000rx_impl();

      bool SetRate(double rate);
      virtual void SetRxFreq(double freq);
      virtual void SetDDCFreq(double freq);
      virtual void SetDDCOutGain(double val);

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
      virtual bool start();
      virtual bool stop();
    };

  } // namespace avs4000
} // namespace gr

#endif /* INCLUDED_AVS4000_AVS4000RX_IMPL_H */


/* -*- c++ -*- */
/* 
 * Copyright 2019 <+YOU OR YOUR COMPANY+>.
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

#ifndef INCLUDED_AVS4000_AVS4000_IMPL_H
#define INCLUDED_AVS4000_AVS4000_IMPL_H

#include <avs4000/avs4000.h>
#include <pmt/pmt.h>


class AVS4000Client;

namespace gr {
  namespace avs4000 {

    class avs4000_impl : public avs4000
    {
     private:
        AVS4000Client *client;
        double rate,rxFreq,txFreq;

        bool Startup();
        pmt::pmt_t id;
     public:
      avs4000_impl(double rate,double rxFreq,double txFreq);
      ~avs4000_impl();

      bool SetRate(double rate);
      double Rate() const { return rate; }

      bool SetRxFreq(double freq);
      double RxFreq() const { return rxFreq; }

      bool SetTxFreq(double freq);
      double TxFreq() const { return txFreq; }

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
          virtual bool start();
          virtual bool stop();
    };

  } // namespace avs4000
} // namespace gr

#endif /* INCLUDED_AVS4000_AVS4000_IMPL_H */


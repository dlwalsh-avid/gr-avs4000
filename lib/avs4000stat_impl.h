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

#ifndef INCLUDED_AVS4000_AVS4000STAT_IMPL_H
#define INCLUDED_AVS4000_AVS4000STAT_IMPL_H

#include <avs4000/avs4000stat.h>

class QWidget;
class QQmlApplicationEngine;
class AVSAPICon;

namespace gr {
  namespace avs4000 {
    class avs4000stat_impl : public avs4000stat
    {
     private:
      // Nothing to declare in this block.
     AVSAPICon *con;
     int dn;
     QQmlApplicationEngine *engine;

     public:
      avs4000stat_impl(const std::string &host,int dn);
      ~avs4000stat_impl();

      QWidget *qwidget();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace avs4000
} // namespace gr

#endif /* INCLUDED_AVS4000_AVS4000STAT_IMPL_H */


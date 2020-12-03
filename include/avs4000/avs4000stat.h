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


#ifndef INCLUDED_AVS4000_AVS4000STAT_H
#define INCLUDED_AVS4000_AVS4000STAT_H

#include <avs4000/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace avs4000 {

    /*!
     * \brief <+description of block+>
     * \ingroup avs4000
     *
     */
    class AVS4000_API avs4000stat : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<avs4000stat> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of avs4000::avs4000stat.
       *
       * To avoid accidental use of raw pointers, avs4000::avs4000stat's
       * constructor is in a private implementation
       * class. avs4000::avs4000stat::make is the public interface for
       * creating new instances.
       */
      static sptr make(const std::string &host,int dn);
    };

  } // namespace avs4000
} // namespace gr

#endif /* INCLUDED_AVS4000_AVS4000STAT_H */


/* -*- c++ -*- */

#define AVS4000_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "avs4000_swig_doc.i"

%{
#include "avs4000/avs4000rx.h"
#include "avs4000/avs4000tx.h"
#include "avs4000/avs4000stat.h"
%}


%include "avs4000/avs4000rx.h"
GR_SWIG_BLOCK_MAGIC2(avs4000, avs4000rx);
%include "avs4000/avs4000tx.h"
GR_SWIG_BLOCK_MAGIC2(avs4000, avs4000tx);
%include "avs4000/avs4000stat.h"
GR_SWIG_BLOCK_MAGIC2(avs4000, avs4000stat);


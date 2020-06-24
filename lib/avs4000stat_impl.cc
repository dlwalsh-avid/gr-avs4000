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
#include "avs4000stat_impl.h"
#include <QtGlobal>
#include "avsapicon.h"
//#include <QQuickWidget>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QObject>

//#define AVSAPI_PORT     12904
#define DEFAULT_HOST    "localhost"

namespace gr {
  namespace avs4000 {

    avs4000stat::sptr
    avs4000stat::make(int dn)
    {
      return gnuradio::get_initial_sptr
        (new avs4000stat_impl(dn));
    }

    /*
     * The private constructor
     */
    avs4000stat_impl::avs4000stat_impl(int dn)
      : gr::block("avs4000stat",
              gr::io_signature::make(0,0,0),
              gr::io_signature::make(0,0,0))
    {
        qDebug("Avs4000Stat...");
        this->con=new AVSAPICon(DEFAULT_HOST,dn);
        this->engine=new QQmlApplicationEngine();
        Q_ASSERT(engine);
        engine->rootContext()->setContextProperty("sock",con);
        const QUrl url(QStringLiteral("qrc:/main.qml"));
//        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
//                         &app, [url](QObject *obj, const QUrl &objUrl) {
//            if (!obj && url == objUrl)
//                QCoreApplication::exit(-1);
//        }, Qt::QueuedConnection);
        engine->load(url);
    }

    /*
     * Our virtual destructor.
     */
    avs4000stat_impl::~avs4000stat_impl()
    {
        if (this->con) {
            delete con;
//            con->disconnect();
        }
        if (this->engine)
            delete engine;
    }

    QWidget *avs4000stat_impl::qwidget()
    {
        qDebug("qwidget called...");
        return nullptr;
//        return new QQuickWidget(engine);
    }

    void
    avs4000stat_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    avs4000stat_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        return 0;
    }

  } /* namespace avs4000 */
} /* namespace gr */


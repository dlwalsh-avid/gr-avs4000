//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "RxSignalClient.h"
#include <QCoreApplication>

static QCoreApplication *app=nullptr;

RxSignalClient::RxSignalClient():QObject(nullptr)
{
    // If there isn't an application instance
    // then we need to create one in order for
    // signals to work properly.  This is needed to support GNU Radio module
    if (QCoreApplication::instance()==nullptr) {
        int argc=1;
        char exe[8];
        strncpy(exe,"rxsigcon",sizeof(exe));
        char *argv[]={ exe,nullptr};
        app=new QCoreApplication(argc,argv);
    }
}

RxSignalClient::~RxSignalClient()
{
    if (app)
        delete app;
}


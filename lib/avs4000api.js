//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
var cfg={}

function connect() {
    var rval=sock.connect();
    if (rval)
        cfg=getAll();
    return rval;
}

function disconnect() {
    return sock.disconnect();
}

function call(req) {
    var rval={};
    if (sock.sendREQ(JSON.stringify(req))) {
        var str=sock.recvRSP();
        if (str.length===0) return false;
        var rsp=JSON.parse(str);
        if (rsp[0]===true)
            rval=rsp[1];
    }
    return rval;
}

function getAll() {
    var req=["get"];
    return call(req);
}

function get(grp) {
    var req=["get",grp];
    return call(req);
}

function update() {
    cfg=getAll();
}

function refStatus() {
    var rval=get("ref");
    var t=rval["Time"];
    return "Time: "+rval.ref.Time.toString();
}

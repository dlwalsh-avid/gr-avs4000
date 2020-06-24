//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include "v49packet.h"

const int V49Packet::Size=8*1024;

V49Packet::V49Packet(bool verbose)
{
    this->valid=false;
    this->samples=nullptr;
    this->verbose=verbose;
}

bool V49Packet::ValidPreamble(const quint8 *data)
{
    const V49Header *h=reinterpret_cast<const V49Header *>(data);
    if (h && h->vrlp==0x56524C50)
        return true;
    if (h==nullptr)
        qWarning("null header ptr!");
    else
        qWarning("Invalid Header: 0x%08X",h->vrlp);
    return false;
}


void V49Packet::Init(const quint8 *data, quint32 len)
{
    Q_ASSERT(len==0x2000);
    this->valid=false;
    Q_ASSERT(len>=(sizeof(V49Header)+sizeof(quint32)));
    CopyHeader(reinterpret_cast<const quint32*>(data));
    samples=reinterpret_cast<const quint32*>(data+sizeof(V49Header));
    samplesLen=(len - sizeof(V49Header) - sizeof(quint32))/sizeof(quint32);
    trailer=samples[samplesLen];    // trailer follows last sample
    Validate();
}

void V49Packet::InitHeaderTrailer(quint8 dn,quint16 stream,
                                  quint16 &nextFrameCount, quint8 &nextPacketCount)
{
    memset(&header,0,sizeof(header));
    header.vrlp=0x56524C50;
    header.pktType=1;
    header.frameSize=0x0800;
    header.frameCount=nextFrameCount++;
    header.pktSize=0x7fd;   // do we need to compute this?
    header.pktCount=nextPacketCount++ % 0x10;
    header.tsi=1;
    header.tsf=1;
    header.streamId=(quint32(dn)<<16)|quint32(stream);
    trailer=0x56454E44;
}

void V49Packet::Make(quint8 dn,quint16 stream,
                     const quint32 *samples, quint32 len,
                     quint16 &nextFrameCount, quint8 &nextPacketCount)
{
    InitHeaderTrailer(dn,stream,nextFrameCount,nextPacketCount);
    this->samples=samples;
    this->samplesLen=len;
}

void V49Packet::Attach(quint8 dn,quint16 stream,
                       quint8 *buffer, quint32 len,
                       quint16 &nextFrameCount, quint8 &nextPacketCount)
{
    Q_ASSERT(len<=0x2000);
    this->valid=false;
    this->buffer=buffer;
    this->bufLen=len;
    InitHeaderTrailer(dn,stream,nextFrameCount,nextPacketCount);
    samples=nullptr;    // no samples for reading
    samplesLen=(bufLen - sizeof(V49Header) - sizeof(quint32))/sizeof(quint32);
    // Copy Header to Buffer
    quint8 *ptr=buffer;
    memcpy(ptr,&header,sizeof(header)); // copy header to buffer
    // Copy Trailer to Buffer
    ptr+=sizeof(header)+samplesLen*sizeof(quint32); // compute buffer location of trailer
    memcpy(ptr,&trailer,sizeof(trailer));
    // now SampleBufer() should point to correct location to write samples
}

void V49Packet::Detach()
{
    this->buffer=nullptr;
    this->bufLen=0;
}

void V49Packet::Copy(quint8 *dest, quint32 len)
{
    Q_ASSERT(len==0x2000);  // assume length is 8KB
    quint8 *ptr=dest;
    memcpy(ptr,&header,sizeof(header));
    ptr+=sizeof(header);
    memcpy(ptr,samples,samplesLen*sizeof(quint32));
    ptr+=samplesLen*sizeof(quint32);
    memcpy(ptr,&trailer,sizeof(trailer));
}

void V49Packet::CopyHeader(const quint32 *data)
{
    const quint32 *src=data;
    quint32 *dest=reinterpret_cast<quint32*>(&header);
    for (quint32 i=0;i<sizeof(header)/sizeof(quint32);i++)
        *dest++=(*src++);
}

bool V49Packet::Validate()
{
    valid=false;
    if (verbose) {
        qDebug("     VRLP = 0x%08X",header.vrlp);
        qDebug("    Frame = 0x%04X 0x%04X",header.frameSize,header.frameCount);
        qDebug("   Packet = 0x%04X 0x%02X",header.pktSize,header.pktCount);
        qDebug("     VEND = 0x%08X",trailer);
    }
    if (FrameSize()!=0x800) return false;
    if (header.vrlp!=0x56524C50) return false;
    if (FrameSize()==0) return false;
    if (RR()!=0x00) return false;
    if (CBit()!=false) return false;
    if (TBit()!=false) return false;
    quint8 type=PType();
    if (!(type==1 || type==3)) return false;
    if (trailer!=0x56454E44) return false;
    valid=true;
    return true;
}

QString V49Packet::Dump(bool showTime) const
{
    QString V=(valid)?"":"INV:";

    QString str;
    if (showTime)
        str.sprintf("%s PT=%d ID=%06X FS=%04X FC=%03X PS=%04X PC=%1X TI=%08X TF=%016llX",
                    qPrintable(V),PType(),StreamID(),FrameSize(),
                    FrameCount(),PacketSize(),PacketCount(),
                    TimeInteger(),TimeFraction());
    else
        str.sprintf("%s PT=%d ID=%06X FS=%04X FC=%03X PS=%04X PC=%1X TSI=%d TSF=%d C=%d T=%d",
                    qPrintable(V),PType(),StreamID(),FrameSize(),
                    FrameCount(),PacketSize(),PacketCount(),
                    TSI(),TSF(),CBit(),TBit());
    return str;
}

QString V49Packet::DumpBuffer() const
{
    QString out;
    if (buffer==nullptr || bufLen==0)
        out.sprintf("no buffer");
    else if (bufLen<0x2000)
        out.sprintf("short buffer");
    else {
        out+="\r\n";
        for (int i=0;i<0x2000/16;i++) {
            QString line;
            line.sprintf("0x%04X:  ",i*16);
            for (int j=0;j<16;j++) {
                QString val;
                val.sprintf("%02X ",buffer[i*16+j]);
                line+=val;
            }
            line+="\r\n";
            out+=line;
        }
    }
    return out;
}

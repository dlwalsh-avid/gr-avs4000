//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#ifndef V49PACKET_H
#define V49PACKET_H

#include <QtGlobal>
#include <QString>

typedef struct __attribute__((packed)) {
    quint32 vrlp;   // 0x56524C50
    quint32 frameSize:20;
    quint16 frameCount:12;
    quint16 pktSize;
    quint8 pktCount:4;
    quint8 tsf:2;
    quint8 tsi:2;
    quint8 rr:2;
    quint8 T:1;
    quint8 C:1;
    quint8 pktType:4;
    quint32 streamId;
    quint32 timeInt;
    quint32 mswTimeFrac;
    quint32 lswTimeFrac;
} V49Header;

class V49Packet
{
public:
    V49Packet(bool verbose=false);
    static const int Size;
    static bool ValidPreamble(const quint8 *data);

    // Initialize V49 packet from data that includes Vita49 header
    void Init(const quint8 *data,quint32 len);
    // Attach V49 packet to existing buffer
    void Attach(quint8 dn, quint16 stream, quint8 *buffer, quint32 len,
                quint16 &nextFrameCount, quint8 &nextPacketCount);
    void Detach();  // Detach from buffer

    // Make Vita49 packet from samples (payload)
    void Make(quint8 dn, quint16 stream, const quint32 *samples, quint32 len,
              quint16 &nextFrameCount, quint8 &nextPacketCount);
    void Copy(quint8 *dest,quint32 len);    // copy V49Packet (header+payload+trailer) to destination buffer

    bool IsValid() const { return valid; }
    bool Validate();    // return true if valid V49 packet
    quint16 FrameCount() const { return header.frameCount; }
    quint32 FrameSize() const { return header.frameSize; }
    quint16 PacketSize() const { return header.pktSize; }
    quint8 PacketCount() const { return header.pktCount; }
    quint8 TSF() const { return header.tsf; }
    quint8 TSI() const { return header.tsi; }
    quint8 RR() const { return header.rr; }
    bool CBit() const { return (header.C==1); }
    bool TBit() const { return (header.T==1); }
    quint8 PType() const { return header.pktType; }
    quint64 TimeFraction() const { return quint64(header.mswTimeFrac)<<32|
                                          quint64(header.lswTimeFrac); }
    quint32 TimeInteger() const { return header.timeInt; }
    quint32 StreamID() const { return header.streamId; }

    const quint32 *Samples() const { return samples; }
    quint32 SamplesLen() const { return samplesLen; }
    quint32 *SampleBuffer() { return (buffer)?reinterpret_cast<quint32*>(&buffer[sizeof(V49Header)]):nullptr; }
    QString Dump(bool showTime=false) const;
    QString DumpBuffer() const;

protected:
    void InitHeaderTrailer(quint8 dn,quint16 stream,
                           quint16 &nextFrameCount,quint8 &nextPacketCount);
    bool verbose;
    bool valid;
    void CopyHeader(const quint32 *data);
    V49Header header;
    const quint32 *samples;
    quint32 samplesLen;
    quint32 trailer;
    quint8 *buffer;
    quint32 bufLen;
};

#endif

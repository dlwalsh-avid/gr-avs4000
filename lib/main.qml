//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import "avs4000api.js" as API


Window {
    visible: true
    width: 260
    height: 440
    title: qsTr("AVS4000")
    color: "black"

    GridLayout{
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        anchors.fill: parent
        columns: (width<height)?1:3
        rows: (width<height)?33:11
        columnSpacing: 2
        rowSpacing: 1
        flow: GridLayout.TopToBottom
        ParamBox { id: dn; name: "Device Number:" }
        ParamBox { id: sn; name: "Serial Number:" }
        ParamBox { id: tempBoard; name: "Board Temp:" }
        ParamBox { id: tempFpgaAmb; name: "FPGA Amb Temp:" }
        ParamBox { id: tempFpgaDie; name: "FPGA Die Temp:" }
        ParamBox { id: vccAux; name: "FPGA Vcc Aux:" }
        ParamBox { id: vccInt; name: "FPGA Vcc Int:" }
        ParamBox { id: vccBRAM; name: "FPGA Vcc BRAM:" }
        ParamBox { id: masterSRMode; name: "Master Mode:" }
        ParamBox { id: masterSampleRate; name: "Master Rate:" }
        ParamBox { id: masterRealSampleRate; name: "Real Master Rate:" }

        ParamBox { id: rxFreq; name: "RX Freq:" }
        ParamBox { id: rxSampleRate; name: "RX Samp Rate:" }
        ParamBox { id: rxRealSampleRate; name: "RX Real Samp Rate:" }
        ParamBox { id: ddcFreq; name: "DDC Freq:" }
        ParamBox { id: ddcRealFreq; name: "DDC Real Freq:" }
//        ParamBox { id: ddcDec; name: "DDC Dec:" }
        ParamBox { id: rxConEnable; name: "RX Con Enable:" }
        ParamBox { id: rxRun; name: "RX Run:" }
        ParamBox { id: rxSamples; name: "RX Samples:" }
        ParamBox { id: rxRate; name: "RX Rate:" }
        ParamBox { id: rxDrops; name: "RX Drops:" }

        ParamBox { id: txFreq; name: "TX Freq:" }
        ParamBox { id: txSampleRate; name: "TX Samp Rate:" }
        ParamBox { id: txRealSampleRate; name: "TX Real Samp Rate:" }
        ParamBox { id: ducFreq; name: "DUC Freq:" }
        ParamBox { id: ducRealFreq; name: "DUC Real Freq:" }
//        ParamBox { id: ducInterp; name: "DUC Interp:" }
        ParamBox { id: txConEnable; name: "TX Con Enable:" }
        ParamBox { id: txRun; name: "TX Run:" }
        ParamBox { id: txSamples; name: "TX Samples:" }
        ParamBox { id: txRate; name: "TX Rate:" }
        ParamBox { id: txUnderruns; name: "TX Underruns:" }
    }

    Timer {
        property int commitCount: -1
        interval: 1000; running: true;repeat: true
        onTriggered: {
            var cfg=API.get(["sysstat","rxstat","txstat"]);
            dn.value=cfg.sysstat.DN;
            sn.value=cfg.sysstat.SN;
            tempBoard.value=cfg.sysstat.BoardTemp.toFixed(2) + " C";
            tempFpgaAmb.value=cfg.sysstat.FpgaAmbTemp.toFixed(2) + " C";
            tempFpgaDie.value=cfg.sysstat.FpgaDieTemp.toFixed(2) + " C";
            vccAux.value=cfg.sysstat.FpgaVccAux.toFixed(3) + " V";
            vccInt.value=cfg.sysstat.FpgaVccInt.toFixed(3) + " V";
            vccBRAM.value=cfg.sysstat.FpgaVccBRAM.toFixed(3) + " V";
            rxRate.value=cfg.rxstat.Rate.toFixed(2) + " MB/s"
            txRate.value=cfg.txstat.Rate.toFixed(2) + " MB/s"
            rxSamples.value=cfg.rxstat.Sample
            txSamples.value=cfg.txstat.Sample
            rxDrops.value=cfg.rxstat.Overflow
            rxDrops.valColor=(cfg.rxstat.Overflow>0)?"red":"lime"
            txUnderruns.value=cfg.txstat.Underflow
            txUnderruns.valColor=(cfg.txstat.Underflow>0)?"red":"lime"
//            ref.value=API.cfg.ref.Time.toString();
            if (commitCount<cfg.sysstat.CommitCount) {
                API.update();
                if (typeof(API.cfg)=='object' &&
                    "master" in API.cfg &&
                    typeof(API.cfg.master)=='object' &&
                    "SampleRateMode" in API.cfg.master) {
                    masterSRMode.value=API.cfg.master.SampleRateMode
                    masterSampleRate.value=(API.cfg.master.SampleRate/1e6).toFixed(6) + " MHz";
                    masterRealSampleRate.value=(API.cfg.master.RealSampleRate/1e6).toFixed(6) + " MHz";
                    rxFreq.value=(API.cfg.rx.Freq/1e6).toFixed(6) + " MHz";
                    rxSampleRate.value=(API.cfg.rx.SampleRate/1e6).toFixed(6) + " MHz";
                    rxRealSampleRate.value=(API.cfg.rx.RealSampleRate/1e6).toFixed(6) + " MHz";
                    ddcFreq.value=API.cfg.ddc.Freq
                    ddcRealFreq.value=API.cfg.ddc.RealFreq
                    rxConEnable.valColor=(API.cfg.rxdata.ConEnable)?'lime':'red'
                    rxConEnable.value=API.cfg.rxdata.ConEnable
                    rxRun.valColor=API.cfg.rxdata.Run?"lime":"red"
                    rxRun.value=API.cfg.rxdata.Run

                    txFreq.value=(API.cfg.tx.Freq/1e6).toFixed(6) + " MHz";
                    txSampleRate.value=(API.cfg.tx.SampleRate/1e6).toFixed(6) + " MHz";
                    txRealSampleRate.value=(API.cfg.tx.RealSampleRate/1e6).toFixed(6) + " MHz";
                    ducFreq.value=API.cfg.duc.Freq
                    ducRealFreq.value=API.cfg.duc.RealFreq
                    txConEnable.value=API.cfg.txdata.ConEnable
                    txConEnable.valColor=API.cfg.txdata.ConEnable?"lime":"red"
                    txRun.value=API.cfg.txdata.Run
                    txRun.valColor=API.cfg.txdata.Run?"lime":"red"
                    commitCount=cfg.sysstat.CommitCount;
                }
            } else if (cfg.sysstat.CommitCount>0)
                commitCount=cfg.sysstat.CommitCount;
        }
    }

    Component.onCompleted: API.connect()
    Component.onDestruction: API.disconnect()
}

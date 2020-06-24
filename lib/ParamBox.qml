//
// Copyright 2020 Avid Systems, Inc
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2
import "avs4000api.js" as API

Rectangle {
    color: "#202020"
    property alias name: name.text
    property alias value: value.text
    property var size: width/18
    property color valColor: "lime"
    property color nameColor: "white"
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumHeight: size
    FontLoader { id: fixedFont; name: "Courier" }

    Text {
        id: name
        font.family: fixedFont.name
        font.pixelSize: parent.size
        color: parent.nameColor
        anchors.right: parent.horizontalCenter
        anchors.rightMargin: -parent.width/16
        anchors.verticalCenter: parent.verticalCenter
        Layout.minimumHeight: font.pixelSize
    }
    Text {
        id: value
        font.family: fixedFont.name
        font.pixelSize: parent.size
        color: parent.valColor
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: parent.width/12
        anchors.verticalCenter: parent.verticalCenter
        Layout.minimumHeight: font.pixelSize
    }
}


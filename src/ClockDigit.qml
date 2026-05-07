import QtQuick

Item {
    id: root
    implicitWidth: currentText.implicitWidth
    implicitHeight: currentText.implicitHeight

    property string digit
    property font textFont
    property color textColor: "white"
    property bool animateUpwards: true

    property string _currentVisibleDigit

    Component.onCompleted: {
        _currentVisibleDigit = root.digit
    }

    clip: true

    Text {
        id: currentText
        text: root._currentVisibleDigit
        font: root.textFont
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: nextText
        text: root.digit
        font: root.textFont
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        y: root.animateUpwards ? root.height : -root.height
    }

    onDigitChanged: {
        if (_currentVisibleDigit === "" || root.digit === _currentVisibleDigit) {
            _currentVisibleDigit = root.digit;
            return;
        }

        slideAnimation.start();
    }

    ParallelAnimation {
        id: slideAnimation

        PropertyAnimation {
            target: currentText
            property: "y"
            from: 0
            to: root.animateUpwards ? -root.height : root.height
            duration: 500
            easing.type: Easing.InOutQuad
        }

        PropertyAnimation {
            target: nextText
            property: "y"
            from: root.animateUpwards ? root.height : -root.height
            to: 0
            duration: 500
            easing.type: Easing.InOutQuad
        }

        onFinished: {
            root._currentVisibleDigit = root.digit;

            currentText.y = 0;
            nextText.y = root.animateUpwards ? root.height : -root.height;
        }
    }
}

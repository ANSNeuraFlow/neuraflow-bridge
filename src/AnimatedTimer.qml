import QtQuick
import QtQuick.Layouts

Item {
    implicitWidth: timer.implicitWidth
    implicitHeight: timer.implicitHeight

    property bool isRunning: true

    property double endTimestamp

    FontLoader {
        id: robotoMonoFont
        source: Qt.resolvedUrl("fonts/" + "roboto_mono.ttf")
    }

    property font clockFont: Qt.font({
                                         "family": robotoMonoFont.name,
                                         "pixelSize": 55,
                                     })

    Row {
        id: timer

        ClockDigit {
            id: h1
            animateUpwards: false
            textFont: clockFont
        }
        ClockDigit {
            id: h2
            animateUpwards: false
            textFont: clockFont
        }

        Text {
            text: ":"
            font: clockFont
            color: "white"
            verticalAlignment: Text.AlignVCenter
        }

        ClockDigit {
            id: m1
            animateUpwards: false
            textFont: clockFont
        }
        ClockDigit {
            id: m2
            animateUpwards: false
            textFont: clockFont
        }

        Text {
            text: ":"
            font: clockFont
            color: "white"
            verticalAlignment: Text.AlignVCenter
        }

        ClockDigit {
            id: s1
            animateUpwards: false
            textFont: clockFont
        }
        ClockDigit {
            id: s2
            animateUpwards: false
            textFont: clockFont
        }
    }

    Timer {
        interval: 1000 // 1 second
        running: isRunning
        repeat: true
        onTriggered: updateTime()
    }

    function updateTime() {
        const now = new Date();
        const timeLeft = endTimestamp - now;

        if (timeLeft <= 0) {
            h1.digit = "0";
            h2.digit = "0";
            m1.digit = "0";
            m2.digit = "0";
            s1.digit = "0";
            s2.digit = "0";

            return;
        }

        const hoursNum = Math.floor((timeLeft / (1000 * 60 * 60)) % 24);
        const minutesNum = Math.floor((timeLeft / (1000 * 60)) % 60);
        const secondsNum = Math.floor((timeLeft / 1000) % 60);

        const hours = hoursNum.toString().padStart(2, "0");
        const minutes = minutesNum.toString().padStart(2, "0");
        const seconds = secondsNum.toString().padStart(2, "0");

        h1.digit = hours[0];
        h2.digit = hours[1];
        m1.digit = minutes[0];
        m2.digit = minutes[1];
        s1.digit = seconds[0];
        s2.digit = seconds[1];
    }

    // Call updateTime() once on startup to avoid a 1-second delay
    Component.onCompleted: {
        updateTime()
    }
}

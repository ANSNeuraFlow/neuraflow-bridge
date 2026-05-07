import QtQuick

Rectangle {
    property color glowColor
    property color originalGlowColor: glowColor
    property bool isFlashing: false

    signal flashAnimationFinished()

    function flashColor(color, restartPulseAnimation = true) {
        pulseAnimation.stop();
        flashAnimation.stop();

        if (!isFlashing) {
            originalGlowColor = glowColor;
            isFlashing = true;
        }

        glowColor = color;
        opacity = 1.0;

        if (restartPulseAnimation) {
            flashAnimation.start();
            return;
        }

        flashAnimationWithStop.start();
    }

    function reset() {
        glowColor = originalGlowColor;
        isFlashing = false;

        pulseAnimation.start();
    }

    id: ledIndicator
    width: 24
    height: 24
    radius: 12
    color: Qt.darker(glowColor, 4)

    // Inner glow for the LED
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.6
        height: parent.height * 0.6
        radius: width / 2
        color: glowColor
    }

    // Pulsing animation
    SequentialAnimation {
        id: pulseAnimation
        running: true
        loops: Animation.Infinite
        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.6
            duration: 1000
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 1000
            easing.type: Easing.InOutQuad
        }
    }

    // Flash animation
    SequentialAnimation {
        id: flashAnimation
        running: false

        // Quick bright flash
        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.5
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.5
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.5
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        // Brief pause at full brightness
        PauseAnimation {
            duration: 200
        }

        // Fade back to normal
        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.8
            duration: 300
        }

        // Restore original color and resume pulsing
        onFinished: {
            glowColor = originalGlowColor;
            isFlashing = false;

            ledIndicator.flashAnimationFinished();

            pulseAnimation.start();
        }
    }

    SequentialAnimation {
        id: flashAnimationWithStop
        running: false

        // Quick bright flash
        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.5
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.5
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 0.5
            duration: 100
        }

        NumberAnimation {
            target: ledIndicator
            property: "opacity"
            to: 1.0
            duration: 100
        }

        onFinished: {
            isFlashing = false;

            ledIndicator.flashAnimationFinished();
        }
    }
}

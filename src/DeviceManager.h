#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QtGlobal>

class QSerialPort;
class QTimer;

/// Cyton/OpenBCI dongle serial: init (v, d, optional c), stream (b/s), 33-byte packets at 250 Hz.
class DeviceManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
  Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
  Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)
  Q_PROPERTY(bool debugMode READ debugMode WRITE setDebugMode NOTIFY debugModeChanged)
  Q_PROPERTY(bool boardReady READ boardReady NOTIFY boardReadyChanged)
  Q_PROPERTY(QString firmwareVersion READ firmwareVersion NOTIFY firmwareVersionChanged)
  Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
  Q_PROPERTY(double effectiveSampleRateHz READ effectiveSampleRateHz NOTIFY effectiveSampleRateHzChanged)

public:
  explicit DeviceManager(QObject *parent = nullptr);

  QStringList availablePorts() const;
  QString selectedPort() const;
  bool connected() const;
  bool streaming() const;
  bool debugMode() const;
  bool boardReady() const { return m_boardReady; }
  QString firmwareVersion() const { return m_firmwareVersion; }
  QString connectionStatus() const { return m_connectionStatus; }
  double effectiveSampleRateHz() const { return m_effectiveSampleRateHz; }

  Q_INVOKABLE void refreshPorts();
  Q_INVOKABLE bool connectDevice();
  Q_INVOKABLE void disconnectDevice();
  Q_INVOKABLE bool startStream();
  Q_INVOKABLE void stopStream();
  Q_INVOKABLE bool autoConnect();

  void setSelectedPort(const QString &portName);
  void setDebugMode(bool debug);

signals:
  void availablePortsChanged();
  void selectedPortChanged();
  void connectedChanged();
  void streamingChanged();
  void debugModeChanged();
  void boardReadyChanged();
  void firmwareVersionChanged();
  void connectionStatusChanged();
  void effectiveSampleRateHzChanged();
  void statusMessage(const QString &message);
  void frameReady(const QByteArray &frame);

private slots:
  void onSerialReadyRead();

private:
  enum class ProtocolState
  {
    Idle,
    WaitingBanner,       // sent 'v', wait for $$$
    WaitingDefaultAck,   // sent 'd', wait for $$$
    Ready,
    Streaming,
  };

  QStringList collectCytonPorts() const;
  bool isSyntheticDebugPort() const;

  void resetProtocolState();
  void sendByte(char cmd);
  void appendSerial(const QByteArray &data);
  void processRxBuffer();
  void handleTextPhase();
  bool tryConsumeTextUntilEot(QString *outChunk = nullptr);
  void onBannerComplete(const QString &chunk);
  void onDefaultSettingsAck(const QString &chunk);
  void drainBinaryPackets();
  bool parseCytonPacket(const QByteArray &packet, QByteArray *outFrame);
  static qint32 interpret24bitSignedMsbFirst(const char *bytes);
  QByteArray buildSyntheticFrame();

  void setBoardReady(bool ready);
  void setFirmwareVersion(const QString &v);
  void setConnectionStatus(const QString &status);
  void setEffectiveSampleRateHz(double hz);

  QSerialPort *m_serialPort;
  QTimer *m_frameTimer;
  QStringList m_availablePorts;
  QString m_selectedPort;
  bool m_connected;
  bool m_streaming;
  bool m_debugMode;
  quint32 m_sequence;

  QByteArray m_rxBuffer;
  ProtocolState m_protocolState{ProtocolState::Idle};
  QString m_textAccumulator;
  bool m_boardReady{false};
  QString m_firmwareVersion;
  QString m_connectionStatus{QStringLiteral("Disconnected")};
  double m_effectiveSampleRateHz{250.0};
};

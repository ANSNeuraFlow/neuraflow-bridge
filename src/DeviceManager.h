#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QtGlobal>

class QSerialPort;
class QTimer;

class DeviceManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
  Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
  Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)

public:
  explicit DeviceManager(QObject *parent = nullptr);

  QStringList availablePorts() const;
  QString selectedPort() const;
  bool connected() const;
  bool streaming() const;

  Q_INVOKABLE void refreshPorts();
  Q_INVOKABLE bool connectDevice();
  Q_INVOKABLE void disconnectDevice();
  Q_INVOKABLE bool startStream();
  Q_INVOKABLE void stopStream();

  void setSelectedPort(const QString &portName);

signals:
  void availablePortsChanged();
  void selectedPortChanged();
  void connectedChanged();
  void streamingChanged();
  void statusMessage(const QString &message);
  void frameReady(const QByteArray &frame);

private:
  QByteArray buildSyntheticFrame();

  QSerialPort *m_serialPort;
  QTimer *m_frameTimer;
  QStringList m_availablePorts;
  QString m_selectedPort;
  bool m_connected;
  bool m_streaming;
  quint32 m_sequence;
};

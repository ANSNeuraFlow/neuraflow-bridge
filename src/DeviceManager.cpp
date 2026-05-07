#include "DeviceManager.h"

#include <QDataStream>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QDateTime>
#include <QRandomGenerator>
#include <QTimer>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent),
      m_serialPort(new QSerialPort(this)),
      m_frameTimer(new QTimer(this)),
      m_connected(false),
      m_streaming(false),
      m_sequence(0)
{
  m_frameTimer->setInterval(10);
  connect(m_frameTimer, &QTimer::timeout, this, [this]()
          { emit frameReady(buildSyntheticFrame()); });

  refreshPorts();
}

QStringList DeviceManager::availablePorts() const
{
  return m_availablePorts;
}

QString DeviceManager::selectedPort() const
{
  return m_selectedPort;
}

bool DeviceManager::connected() const
{
  return m_connected;
}

bool DeviceManager::streaming() const
{
  return m_streaming;
}

void DeviceManager::refreshPorts()
{
  QStringList ports;
  const auto serialPorts = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : serialPorts)
  {
    ports.append(info.portName());
  }

  if (m_availablePorts == ports)
  {
    return;
  }

  m_availablePorts = ports;
  emit availablePortsChanged();

  if (m_selectedPort.isEmpty() && !m_availablePorts.isEmpty())
  {
    m_selectedPort = m_availablePorts.first();
    emit selectedPortChanged();
  }
}

bool DeviceManager::connectDevice()
{
  if (m_selectedPort.isEmpty())
  {
    emit statusMessage(QStringLiteral("Select a serial port before connecting"));
    return false;
  }

  if (m_connected)
  {
    return true;
  }

  if (m_serialPort->isOpen())
  {
    m_serialPort->close();
  }

  m_serialPort->setPortName(m_selectedPort);
  m_serialPort->setBaudRate(115200);
  if (!m_serialPort->open(QIODevice::ReadWrite))
  {
    emit statusMessage(QStringLiteral("Failed to open serial port %1: %2").arg(m_selectedPort, m_serialPort->errorString()));
    return false;
  }

  m_connected = true;
  emit connectedChanged();
  emit statusMessage(QStringLiteral("Device connected on %1").arg(m_selectedPort));
  return true;
}

void DeviceManager::disconnectDevice()
{
  if (!m_connected)
  {
    return;
  }

  if (m_streaming)
  {
    stopStream();
  }

  m_connected = false;
  if (m_serialPort->isOpen())
  {
    m_serialPort->close();
  }
  emit connectedChanged();
  emit statusMessage(QStringLiteral("Device disconnected"));
}

bool DeviceManager::startStream()
{
  if (!m_connected)
  {
    emit statusMessage(QStringLiteral("Connect device before starting stream"));
    return false;
  }

  if (m_streaming)
  {
    return true;
  }

  m_streaming = true;
  m_sequence = 0;
  m_frameTimer->start();
  emit streamingChanged();
  emit statusMessage(QStringLiteral("Device stream started"));
  return true;
}

void DeviceManager::stopStream()
{
  if (!m_streaming)
  {
    return;
  }

  m_streaming = false;
  m_frameTimer->stop();
  emit streamingChanged();
  emit statusMessage(QStringLiteral("Device stream stopped"));
}

void DeviceManager::setSelectedPort(const QString &portName)
{
  if (m_selectedPort == portName)
  {
    return;
  }

  m_selectedPort = portName;
  emit selectedPortChanged();
}

QByteArray DeviceManager::buildSyntheticFrame()
{
  QByteArray frame;
  frame.reserve(8 + 4 + (8 * 4));

  QDataStream stream(&frame, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);

  const qint64 timestampMs = QDateTime::currentMSecsSinceEpoch();
  stream << timestampMs;
  stream << m_sequence++;

  for (int i = 0; i < 8; ++i)
  {
    const float value = static_cast<float>(QRandomGenerator::global()->generateDouble() * 500.0 - 250.0);
    stream << value;
  }

  return frame;
}

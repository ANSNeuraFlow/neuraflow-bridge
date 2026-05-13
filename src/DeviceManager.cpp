#include "DeviceManager.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QSerialPort>
#include <QSerialPortInfo>

#include <cmath>

namespace
{
  constexpr char kCmdSoftReset = 'v';
  constexpr char kCmdDefaultChannels = 'd';
  constexpr char kCmdEightChannels = 'c';
  constexpr char kCmdStartStream = 'b';
  constexpr char kCmdStopStream = 's';

  constexpr quint8 kPacketHeader = 0xA0;
  constexpr int kPacketSize = 33;

  // ADS1299 default gain 24: uV per count = 4.5 / gain / (2^23 - 1) * 1e6
  constexpr double kMicrovoltsPerCountGain24 =
      (4.5 / 24.0 / static_cast<double>((1 << 23) - 1)) * 1e6;

  constexpr double kCytonDefaultSampleRateHz = 250.0;

  QString normalizedSerialPath(const QString &portName)
  {
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    if (portName.startsWith(QStringLiteral("/dev/")))
    {
      return portName;
    }
    return QStringLiteral("/dev/") + portName;
#else
    return portName;
#endif
  }
} // namespace

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent),
      m_serialPort(new QSerialPort(this)),
      m_connected(false),
      m_streaming(false),
      m_debugMode(false)
{
  connect(m_serialPort, &QSerialPort::readyRead, this, &DeviceManager::onSerialReadyRead);

  setEffectiveSampleRateHz(kCytonDefaultSampleRateHz);
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

bool DeviceManager::debugMode() const
{
  return m_debugMode;
}

QStringList DeviceManager::collectCytonPorts() const
{
  // Description prefixes reported by the OpenBCI Cyton dongle across platforms.
  // Linux  : "FT231X USB UART" (FTDI kernel driver)
  // Windows: CP210x driver → "Silicon Labs CP210x USB to UART Bridge",
  //                           "USB-SERIAL CP2104", "CP2102 USB to UART Bridge Controller"
  //          FTDI driver  → "USB Serial Port", "FT231X USB UART"
  // macOS  : "FT231X USB UART" or "CP2102 USB to UART Bridge Controller"
  static const QStringList kDongleDescPrefixes = {
      QStringLiteral("FT231X USB UART"),
      QStringLiteral("VCP"),
      // Windows CP210x variants
      QStringLiteral("Silicon Labs CP210x"),
      QStringLiteral("CP210"),                 // CP2102, CP2104, CP2109…
      QStringLiteral("USB-SERIAL CP"),
      // Windows FTDI variants
      QStringLiteral("USB Serial Port"),
  };

  // Known USB Vendor IDs for OpenBCI Cyton dongle hardware.
  // VID 0x10C4 = Silicon Labs (CP2104)
  // VID 0x0403 = FTDI
  static const QList<quint16> kDongleVids = { 0x10C4, 0x0403 };

  QStringList results;

  const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos)
  {
    const QString desc = info.description();

    // Primary match: description prefix
    bool matchesDongle = false;
    for (const QString &prefix : kDongleDescPrefixes)
    {
      if (desc.startsWith(prefix, Qt::CaseInsensitive))
      {
        matchesDongle = true;
        break;
      }
    }

    // Fallback: VID/PID allowlist (catches renamed/generic drivers on Windows)
    if (!matchesDongle && info.hasVendorIdentifier())
    {
      for (quint16 vid : kDongleVids)
      {
        if (info.vendorIdentifier() == vid)
        {
          matchesDongle = true;
          qDebug() << "DeviceManager: matched dongle by VID"
                   << Qt::hex << vid
                   << "port" << info.portName()
                   << "desc" << desc;
          break;
        }
      }
    }

    if (!matchesDongle)
    {
      continue;
    }

    QString sysName = info.portName();
#if defined(Q_OS_MACOS)
    if (sysName.startsWith(QStringLiteral("tty")))
    {
      continue;
    }
#endif
    const QString path = normalizedSerialPath(sysName);
    if (!results.contains(path))
    {
      results.append(path);
    }
  }

#if defined(Q_OS_LINUX)
  if (m_debugMode)
  {
    QDir devDir(QStringLiteral("/dev"));
    const QStringList devEntries = devDir.entryList(QDir::System | QDir::NoDotAndDotDot);
    for (const QString &name : devEntries)
    {
      if (name.startsWith(QStringLiteral("tnt")))
      {
        const QString path = QStringLiteral("/dev/") + name;
        if (!results.contains(path))
        {
          results.append(path);
          qDebug() << "DeviceManager: DEBUG virtual tty0tty-style port" << path;
        }
      }
    }
  }
#endif

  return results;
}

void DeviceManager::setDebugMode(bool debug)
{
  if (m_debugMode != debug)
  {
    m_debugMode = debug;
    emit debugModeChanged();
  }
  refreshPorts();
}

void DeviceManager::refreshPorts()
{
  const QStringList ports = collectCytonPorts();

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

bool DeviceManager::autoConnect()
{
  refreshPorts();

  if (m_availablePorts.isEmpty())
  {
    emit statusMessage(QStringLiteral("No Cyton dongles found"));
    return false;
  }

  setSelectedPort(m_availablePorts.first());
  return connectDevice();
}

void DeviceManager::resetProtocolState()
{
  m_rxBuffer.clear();
  m_textAccumulator.clear();
  m_protocolState = ProtocolState::Idle;
}

void DeviceManager::setBoardReady(bool ready)
{
  if (m_boardReady == ready)
  {
    return;
  }
  m_boardReady = ready;
  emit boardReadyChanged();
}

void DeviceManager::setFirmwareVersion(const QString &v)
{
  if (m_firmwareVersion == v)
  {
    return;
  }
  m_firmwareVersion = v;
  emit firmwareVersionChanged();
}

void DeviceManager::setConnectionStatus(const QString &status)
{
  if (m_connectionStatus == status)
  {
    return;
  }
  m_connectionStatus = status;
  emit connectionStatusChanged();
}

void DeviceManager::setEffectiveSampleRateHz(double hz)
{
  if (std::fabs(m_effectiveSampleRateHz - hz) < 1e-9)
  {
    return;
  }
  m_effectiveSampleRateHz = hz;
  emit effectiveSampleRateHzChanged();
}

void DeviceManager::sendByte(char cmd)
{
  if (!m_serialPort->isOpen())
  {
    return;
  }
  const QByteArray payload(1, cmd);
  m_serialPort->write(payload);
  m_serialPort->flush();
}

void DeviceManager::appendSerial(const QByteArray &data)
{
  if (data.isEmpty())
  {
    return;
  }
  m_rxBuffer.append(data);
  processRxBuffer();
}

void DeviceManager::processRxBuffer()
{
  if (m_protocolState == ProtocolState::Streaming)
  {
    drainBinaryPackets();
    return;
  }

  handleTextPhase();
}

void DeviceManager::handleTextPhase()
{
  QString chunk;
  while (tryConsumeTextUntilEot(&chunk))
  {
    if (m_protocolState == ProtocolState::WaitingBanner)
    {
      onBannerComplete(chunk);
    }
    else if (m_protocolState == ProtocolState::WaitingDefaultAck)
    {
      onDefaultSettingsAck(chunk);
    }
  }
}

bool DeviceManager::tryConsumeTextUntilEot(QString *outChunk)
{
  static const QByteArray kEot("$$$");

  const int idx = m_rxBuffer.indexOf(kEot);
  if (idx < 0)
  {
    return false;
  }

  const QByteArray rawLine = m_rxBuffer.left(idx + kEot.size());
  m_rxBuffer.remove(0, idx + kEot.size());

  QString text = QString::fromLatin1(rawLine);
  text.remove(QStringLiteral("$$$"));

  if (outChunk)
  {
    *outChunk = text;
  }
  return true;
}

void DeviceManager::onBannerComplete(const QString &chunk)
{
  static const QRegularExpression kFwRegex(
      QStringLiteral(R"(Firmware:\s*(v[\d.]+))"),
      QRegularExpression::CaseInsensitiveOption);

  const auto match = kFwRegex.match(chunk);
  if (match.hasMatch())
  {
    setFirmwareVersion(match.captured(1));
  }
  else
  {
    setFirmwareVersion(QString());
  }

  m_protocolState = ProtocolState::WaitingDefaultAck;
  sendByte(kCmdDefaultChannels);
  setConnectionStatus(QStringLiteral("Applying default channel settings…"));
  emit statusMessage(QStringLiteral("Cyton ready; applying default channel settings"));
}

void DeviceManager::onDefaultSettingsAck(const QString &chunk)
{
  Q_UNUSED(chunk);
  // Align with OpenBCI GUI: prefer 8-channel mode when Daisy may be present.
  sendByte(kCmdEightChannels);

  m_protocolState = ProtocolState::Ready;
  setBoardReady(true);
  setConnectionStatus(QStringLiteral("Board ready"));
  emit statusMessage(QStringLiteral("Cyton configured (default channels, 8ch mode)"));
}

void DeviceManager::onSerialReadyRead()
{
  if (!m_serialPort->isOpen())
  {
    return;
  }
  appendSerial(m_serialPort->readAll());
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

  resetProtocolState();
  setBoardReady(false);
  setFirmwareVersion(QString());

  m_serialPort->setPortName(m_selectedPort);
  m_serialPort->setBaudRate(115200);
  m_serialPort->setDataBits(QSerialPort::Data8);
  m_serialPort->setParity(QSerialPort::NoParity);
  m_serialPort->setStopBits(QSerialPort::OneStop);
  m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

  if (!m_serialPort->open(QIODevice::ReadWrite))
  {
    emit statusMessage(QStringLiteral("Failed to open serial port %1: %2").arg(m_selectedPort, m_serialPort->errorString()));
    setConnectionStatus(QStringLiteral("Open failed"));
    return false;
  }

  m_connected = true;
  emit connectedChanged();

  setEffectiveSampleRateHz(kCytonDefaultSampleRateHz);
  m_protocolState = ProtocolState::WaitingBanner;
  setConnectionStatus(QStringLiteral("Waiting for board…"));
  emit statusMessage(QStringLiteral("Serial open; soft-reset (v)…"));

  sendByte(kCmdSoftReset);
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
  setBoardReady(false);
  setFirmwareVersion(QString());
  resetProtocolState();
  setConnectionStatus(QStringLiteral("Disconnected"));
  setEffectiveSampleRateHz(kCytonDefaultSampleRateHz);

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

  if (!m_boardReady)
  {
    emit statusMessage(QStringLiteral("Board not ready yet"));
    return false;
  }

  if (m_streaming)
  {
    return true;
  }

  sendByte(kCmdStartStream);
  m_protocolState = ProtocolState::Streaming;
  m_rxBuffer.clear();

  m_streaming = true;
  emit streamingChanged();
  setConnectionStatus(QStringLiteral("Streaming"));
  emit statusMessage(QStringLiteral("Cyton stream started (b)"));
  return true;
}

void DeviceManager::stopStream()
{
  if (!m_streaming)
  {
    return;
  }

  if (m_serialPort->isOpen())
  {
    sendByte(kCmdStopStream);
  }

  m_streaming = false;
  emit streamingChanged();

  if (m_connected && m_boardReady)
  {
    m_protocolState = ProtocolState::Ready;
    m_rxBuffer.clear();
    setConnectionStatus(QStringLiteral("Board ready"));
  }

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

qint32 DeviceManager::interpret24bitSignedMsbFirst(const char *bytes)
{
  quint32 u =
      (static_cast<quint8>(bytes[0]) << 16) |
      (static_cast<quint8>(bytes[1]) << 8) |
      static_cast<quint8>(bytes[2]);

  qint32 v = static_cast<qint32>(u & 0x00FFFFFFu);
  if ((v & 0x00800000) != 0)
  {
    v |= static_cast<qint32>(0xFF000000);
  }
  return v;
}

bool DeviceManager::parseCytonPacket(const QByteArray &packet, QByteArray *outFrame)
{
  if (!outFrame || packet.size() != kPacketSize)
  {
    return false;
  }

  const quint8 footer = static_cast<quint8>(packet.at(kPacketSize - 1));
  if (footer < 0xC0 || footer > 0xCF)
  {
    return false;
  }

  QByteArray frame;
  frame.reserve(static_cast<int>(sizeof(qint64) + sizeof(quint32) + 8 * sizeof(float)));

  QDataStream stream(&frame, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::LittleEndian);

  const qint64 timestampMs = QDateTime::currentMSecsSinceEpoch();
  const quint32 seq = static_cast<quint8>(packet.at(1));

  stream << timestampMs << seq;

  int offset = 2;
  for (int ch = 0; ch < 8; ++ch)
  {
    const qint32 counts = interpret24bitSignedMsbFirst(packet.constData() + offset);
    offset += 3;
    const float microvolts = static_cast<float>(static_cast<double>(counts) * kMicrovoltsPerCountGain24);
    stream << microvolts;
  }

  *outFrame = frame;
  return true;
}

void DeviceManager::drainBinaryPackets()
{
  while (true)
  {
    const int start = m_rxBuffer.indexOf(static_cast<char>(kPacketHeader));
    if (start < 0)
    {
      if (m_rxBuffer.size() > 4096)
      {
        m_rxBuffer.clear();
      }
      break;
    }

    if (start > 0)
    {
      m_rxBuffer.remove(0, start);
    }

    if (m_rxBuffer.size() < kPacketSize)
    {
      break;
    }

    const QByteArray candidate = m_rxBuffer.left(kPacketSize);
    const quint8 footer = static_cast<quint8>(candidate.at(kPacketSize - 1));
    if (footer < 0xC0 || footer > 0xCF)
    {
      m_rxBuffer.remove(0, 1);
      continue;
    }

    QByteArray frame;
    if (!parseCytonPacket(candidate, &frame))
    {
      m_rxBuffer.remove(0, 1);
      continue;
    }

    m_rxBuffer.remove(0, kPacketSize);
    emit frameReady(frame);
  }
}

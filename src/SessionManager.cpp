#include "SessionManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QSysInfo>

#include "AuthManager.h"
#include "BridgeDeviceClient.h"
#include "DeepLinkHandler.h"
#include "DeviceManager.h"
#include "StreamUploader.h"
#include "TokenStore.h"

namespace
{
  QString bridgeApiPlatform()
  {
    const QString kernel = QSysInfo::kernelType().toLower();
    if (kernel == QStringLiteral("linux"))
      return QStringLiteral("linux");
    if (kernel == QStringLiteral("darwin"))
      return QStringLiteral("macos");
    if (kernel == QStringLiteral("winnt"))
      return QStringLiteral("windows");
    return QStringLiteral("linux");
  }
}

SessionManager::SessionManager(AuthManager *authManager,
                               DeviceManager *deviceManager,
                               StreamUploader *streamUploader,
                               BridgeDeviceClient *bridgeDeviceClient,
                               TokenStore *tokenStore,
                               DeepLinkHandler *deepLinkHandler,
                               QObject *parent)
    : QObject(parent),
      m_authManager(authManager),
      m_deviceManager(deviceManager),
      m_streamUploader(streamUploader),
      m_bridgeDeviceClient(bridgeDeviceClient),
      m_tokenStore(tokenStore),
      m_deepLinkHandler(deepLinkHandler),
      m_state(QStringLiteral("idle"))
{
  connect(m_authManager, &AuthManager::authStateChanged, this, &SessionManager::stateChanged);
  connect(m_authManager, &AuthManager::statusMessageChanged, this, [this]()
          { setStatusMessage(m_authManager->statusMessage()); });
  connect(m_authManager, &AuthManager::authenticatedChanged, this, &SessionManager::authenticatedChanged);

  connect(m_deviceManager, &DeviceManager::availablePortsChanged, this, &SessionManager::availablePortsChanged);
  connect(m_deviceManager, &DeviceManager::selectedPortChanged, this, &SessionManager::selectedPortChanged);
  connect(m_deviceManager, &DeviceManager::connectedChanged, this, &SessionManager::deviceConnectedChanged);
  connect(m_deviceManager, &DeviceManager::streamingChanged, this, &SessionManager::stateChanged);
  connect(m_deviceManager, &DeviceManager::statusMessage, this, &SessionManager::setStatusMessage);

  connect(m_streamUploader, &StreamUploader::connectedChanged, this, &SessionManager::streamConnectedChanged);
  connect(m_streamUploader, &StreamUploader::statusMessage, this, &SessionManager::setStatusMessage);

  connect(m_deviceManager, &DeviceManager::frameReady, this, [this](const QByteArray &frame)
          {
    if (m_streamUploader)
    {
      m_streamUploader->enqueueFrame(frame);
    } });

  connect(m_bridgeDeviceClient, &BridgeDeviceClient::registerSucceeded, this, [this](const QString &deviceId)
          { setStatusMessage(QStringLiteral("Bridge device registered: %1").arg(deviceId)); });
  connect(m_bridgeDeviceClient, &BridgeDeviceClient::registerFailed, this, [this](const QString &error)
          { setStatusMessage(error); });

  connect(m_authManager, &AuthManager::tokenReady, this, [this]()
          {
        if (!m_tokenStore || !m_tokenStore->hasValidToken())
        {
            return;
        }
        const QString token = m_tokenStore->token();
        const QString apiUrl = m_bridgeApiSettings.value(QStringLiteral("apiUrl")).toString();
        const QString devicesPath = m_bridgeApiSettings.value(QStringLiteral("devicesPath")).toString();
        m_bridgeDeviceClient->registerDevice(
            apiUrl,
            devicesPath,
            token,
            QSysInfo::prettyProductName(),
            bridgeApiPlatform(),
            QCoreApplication::applicationVersion()); });

  connect(m_bridgeDeviceClient, &BridgeDeviceClient::listSucceeded, this, [this](const QStringList &deviceIds)
          { setStatusMessage(QStringLiteral("Known bridge devices: %1").arg(deviceIds.join(QStringLiteral(", ")))); });
}

QString SessionManager::state() const
{
  if (m_authManager && !m_authManager->authenticated())
  {
    return m_authManager->authState();
  }
  if (m_deviceManager && m_deviceManager->streaming())
  {
    return QStringLiteral("streaming");
  }
  if (m_deviceManager && m_deviceManager->connected())
  {
    return QStringLiteral("device_connected");
  }
  return m_state;
}

QString SessionManager::statusMessage() const
{
  return m_statusMessage;
}

bool SessionManager::authenticated() const
{
  return m_authManager && m_authManager->authenticated();
}

QStringList SessionManager::availablePorts() const
{
  return m_deviceManager ? m_deviceManager->availablePorts() : QStringList();
}

QString SessionManager::selectedPort() const
{
  return m_deviceManager ? m_deviceManager->selectedPort() : QString();
}

bool SessionManager::deviceConnected() const
{
  return m_deviceManager && m_deviceManager->connected();
}

bool SessionManager::streamConnected() const
{
  return m_streamUploader && m_streamUploader->connected();
}

void SessionManager::beginLogin(const QVariantMap &bridgeApiSettings)
{
  if (!m_authManager)
  {
    return;
  }

  m_bridgeApiSettings = bridgeApiSettings;

  const QString webUrl = bridgeApiSettings.value(QStringLiteral("webUrl")).toString();
  const QString apiUrl = bridgeApiSettings.value(QStringLiteral("apiUrl")).toString();
  const QString authStartPath = bridgeApiSettings.value(QStringLiteral("authStartPath")).toString();
  const QString authTokenPath = bridgeApiSettings.value(QStringLiteral("authTokenPath")).toString();
  const int codeTtlSeconds = bridgeApiSettings.value(QStringLiteral("codeTtlSeconds"), 120).toInt();
  QStringList allowedClientIds = bridgeApiSettings.value(QStringLiteral("allowedClientIds")).toStringList();
  if (allowedClientIds.isEmpty())
  {
    const QString configured = bridgeApiSettings.value(QStringLiteral("allowedClientIds")).toString();
    if (!configured.trimmed().isEmpty())
    {
      const QStringList splitValues = configured.split(',', Qt::SkipEmptyParts);
      for (const QString &value : splitValues)
      {
        allowedClientIds.append(value.trimmed());
      }
    }
  }

  m_authManager->configureAuthConstraints(codeTtlSeconds, allowedClientIds);

  if (m_authManager->beginLogin(webUrl, apiUrl, authStartPath, authTokenPath))
  {
    setState(QStringLiteral("auth_in_progress"));
  }
}

void SessionManager::refreshPorts()
{
  if (m_deviceManager)
  {
    m_deviceManager->refreshPorts();
  }
}

void SessionManager::connectDevice()
{
  if (m_deviceManager && m_deviceManager->connectDevice())
  {
    setState(QStringLiteral("device_connected"));
  }
}

void SessionManager::disconnectDevice()
{
  if (m_deviceManager)
  {
    m_deviceManager->disconnectDevice();
  }
  setState(QStringLiteral("authenticated"));
}

void SessionManager::startStreaming(const QVariantMap &bridgeApiSettings)
{
  m_bridgeApiSettings = bridgeApiSettings;

  if (!m_tokenStore || !m_tokenStore->hasValidToken())
  {
    setStatusMessage(QStringLiteral("Authenticate before starting stream"));
    return;
  }

  if (!m_deviceManager || !m_deviceManager->startStream())
  {
    return;
  }

  const QString wsUrl = bridgeApiSettings.value(QStringLiteral("streamWsUrl")).toString();
  if (wsUrl.isEmpty())
  {
    setStatusMessage(QStringLiteral("Missing stream websocket URL in configuration"));
    m_deviceManager->stopStream();
    return;
  }

  m_streamUploader->connectToBackend(wsUrl, m_tokenStore->token());
  setState(QStringLiteral("streaming"));
}

void SessionManager::stopStreaming()
{
  if (m_deviceManager)
  {
    m_deviceManager->stopStream();
  }
  if (m_streamUploader)
  {
    m_streamUploader->disconnectFromBackend();
  }
  setState(QStringLiteral("device_connected"));
}

void SessionManager::setSelectedPort(const QString &portName)
{
  if (m_deviceManager)
  {
    m_deviceManager->setSelectedPort(portName);
  }
}

void SessionManager::setState(const QString &state)
{
  if (m_state == state)
  {
    return;
  }

  m_state = state;
  emit stateChanged();
}

void SessionManager::setStatusMessage(const QString &message)
{
  if (m_statusMessage == message)
  {
    return;
  }

  m_statusMessage = message;
  emit statusMessageChanged();
}

#include "SessionManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QMetaObject>
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
                               const QVariantMap &bridgeRuntimeSettings,
                               QObject *parent)
    : QObject(parent),
      m_authManager(authManager),
      m_deviceManager(deviceManager),
      m_streamUploader(streamUploader),
      m_bridgeDeviceClient(bridgeDeviceClient),
      m_tokenStore(tokenStore),
      m_deepLinkHandler(deepLinkHandler),
      m_bridgeApiSettings(bridgeRuntimeSettings),
      m_state(QStringLiteral("idle")),
      m_autoConnectOnStartup(bridgeRuntimeSettings.value(QStringLiteral("autoConnectOnStartup")).toBool()),
      m_autoConnectStartupDone(false)
{
  connect(m_authManager, &AuthManager::authStateChanged, this, &SessionManager::stateChanged);
  connect(m_authManager, &AuthManager::authStateChanged, this, &SessionManager::notifyUiStateChanged);
  connect(m_authManager, &AuthManager::statusMessageChanged, this, [this]()
          { setStatusMessage(m_authManager->statusMessage()); });
  connect(m_authManager, &AuthManager::authenticatedChanged, this, &SessionManager::authenticatedChanged);
  connect(m_authManager, &AuthManager::authenticatedChanged, this, &SessionManager::notifyUiStateChanged);
  connect(m_authManager, &AuthManager::authenticatedChanged, this, &SessionManager::handleAuthenticatedForAutoConnect);
  connect(m_authManager, &AuthManager::authFailed, this, &SessionManager::notifyUiStateChanged);

  connect(m_deviceManager, &DeviceManager::availablePortsChanged, this, &SessionManager::availablePortsChanged);
  connect(m_deviceManager, &DeviceManager::selectedPortChanged, this, &SessionManager::selectedPortChanged);
  connect(m_deviceManager, &DeviceManager::connectedChanged, this, &SessionManager::deviceConnectedChanged);
  connect(m_deviceManager, &DeviceManager::streamingChanged, this, &SessionManager::stateChanged);
  connect(m_deviceManager, &DeviceManager::streamingChanged, this, &SessionManager::deviceStreamingChanged);
  connect(m_deviceManager, &DeviceManager::statusMessage, this, &SessionManager::setStatusMessage);
  connect(m_deviceManager, &DeviceManager::boardReadyChanged, this, &SessionManager::boardReadyChanged);
  connect(m_deviceManager, &DeviceManager::firmwareVersionChanged, this, &SessionManager::firmwareVersionChanged);
  connect(m_deviceManager, &DeviceManager::connectionStatusChanged, this, &SessionManager::connectionStatusChanged);

  connect(m_streamUploader, &StreamUploader::connectedChanged, this, &SessionManager::streamConnectedChanged);
  connect(m_streamUploader, &StreamUploader::statusMessage, this, &SessionManager::setStatusMessage);

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

  if (m_deepLinkHandler)
  {
    connect(m_deepLinkHandler, &DeepLinkHandler::payloadChanged, this, &SessionManager::autoLoginIfReady);
  }

  notifyUiStateChanged();

  QMetaObject::invokeMethod(this, &SessionManager::handleAuthenticatedForAutoConnect, Qt::QueuedConnection);
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

QString SessionManager::uiState() const
{
  if (authenticated())
  {
    return QStringLiteral("ready");
  }
  if (!m_authManager)
  {
    return QStringLiteral("splash");
  }

  const QString authState = m_authManager->authState();
  if (authState == QStringLiteral("waiting_callback") || authState == QStringLiteral("exchanging_token"))
  {
    return QStringLiteral("authenticating");
  }
  if (authState == QStringLiteral("error"))
  {
    return QStringLiteral("error");
  }

  return QStringLiteral("splash");
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

bool SessionManager::deviceStreaming() const
{
  return m_deviceManager && m_deviceManager->streaming();
}

bool SessionManager::boardReady() const
{
  return m_deviceManager && m_deviceManager->boardReady();
}

QString SessionManager::firmwareVersion() const
{
  return m_deviceManager ? m_deviceManager->firmwareVersion() : QString();
}

QString SessionManager::connectionStatus() const
{
  return m_deviceManager ? m_deviceManager->connectionStatus() : QStringLiteral("Disconnected");
}

bool SessionManager::streamConnected() const
{
  return m_streamUploader && m_streamUploader->connected();
}

void SessionManager::notifyUiStateChanged()
{
  emit uiStateChanged();
}

void SessionManager::autoLoginIfReady()
{
  if (!m_deepLinkHandler || !m_deepLinkHandler->hasConnectPayload())
  {
    return;
  }
  if (authenticated())
  {
    return;
  }
  if (!m_authManager)
  {
    return;
  }

  const QString authState = m_authManager->authState();
  if (authState == QStringLiteral("waiting_callback") || authState == QStringLiteral("exchanging_token"))
  {
    return;
  }

  beginLogin();
}

void SessionManager::beginLogin()
{
  if (!m_authManager || !m_deepLinkHandler)
  {
    return;
  }

  if (!m_deepLinkHandler->hasConnectPayload())
  {
    const QString selfClientId = m_bridgeApiSettings.value(QStringLiteral("selfClientId")).toString().trimmed();
    if (selfClientId.isEmpty())
    {
      setStatusMessage(QStringLiteral("Missing selfClientId in bridge configuration"));
      notifyUiStateChanged();
      return;
    }
    m_deepLinkHandler->blockSignals(true);
    m_deepLinkHandler->seedSelfInitiated(selfClientId);
    m_deepLinkHandler->blockSignals(false);
    if (!m_deepLinkHandler->hasConnectPayload())
    {
      notifyUiStateChanged();
      return;
    }
  }

  const QString webUrl = m_bridgeApiSettings.value(QStringLiteral("webUrl")).toString();
  const QString apiUrl = m_bridgeApiSettings.value(QStringLiteral("apiUrl")).toString();
  const QString authStartPath = m_bridgeApiSettings.value(QStringLiteral("authStartPath")).toString();
  const QString authTokenPath = m_bridgeApiSettings.value(QStringLiteral("authTokenPath")).toString();
  const int codeTtlSeconds = m_bridgeApiSettings.value(QStringLiteral("codeTtlSeconds"), 120).toInt();
  QStringList allowedClientIds = m_bridgeApiSettings.value(QStringLiteral("allowedClientIds")).toStringList();
  if (allowedClientIds.isEmpty())
  {
    const QString configured = m_bridgeApiSettings.value(QStringLiteral("allowedClientIds")).toString();
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

  notifyUiStateChanged();
}

void SessionManager::logout()
{
  if (m_deviceManager && m_deviceManager->connected())
  {
    disconnectDevice();
  }
  if (m_authManager)
  {
    m_authManager->logout();
  }
  setState(QStringLiteral("idle"));
  notifyUiStateChanged();
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

void SessionManager::autoConnectDevice()
{
  if (m_deviceManager && m_deviceManager->autoConnect())
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

void SessionManager::startStreaming()
{
  if (!m_deviceManager || !m_deviceManager->startStream())
  {
    return;
  }

  if (m_deviceManager->connected())
  {
    setState(QStringLiteral("streaming"));
  }
}

void SessionManager::stopStreaming()
{
  if (m_deviceManager)
  {
    m_deviceManager->stopStream();
  }

  if (m_deviceManager && m_deviceManager->connected())
  {
    setState(QStringLiteral("device_connected"));
  }
  else
  {
    setState(QStringLiteral("authenticated"));
  }
}

void SessionManager::publishStatus(const QString &message)
{
  setStatusMessage(message);
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

void SessionManager::handleAuthenticatedForAutoConnect()
{
  if (!m_authManager || !m_authManager->authenticated())
  {
    m_autoConnectStartupDone = false;
    return;
  }

  if (!m_autoConnectOnStartup || m_autoConnectStartupDone)
  {
    return;
  }

  if (m_deviceManager && m_deviceManager->connected())
  {
    return;
  }

  m_autoConnectStartupDone = true;

  QMetaObject::invokeMethod(
      this,
      [this]()
      {
        if (m_deviceManager && !m_deviceManager->connected())
        {
          autoConnectDevice();
        }
      },
      Qt::QueuedConnection);
}

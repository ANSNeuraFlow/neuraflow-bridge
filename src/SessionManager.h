#pragma once

#include <QObject>
#include <QVariantMap>

class AuthManager;
class DeviceManager;
class StreamUploader;
class BridgeDeviceClient;
class TokenStore;
class DeepLinkHandler;

class SessionManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString uiState READ uiState NOTIFY uiStateChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
  Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
  Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
  Q_PROPERTY(bool deviceConnected READ deviceConnected NOTIFY deviceConnectedChanged)
  Q_PROPERTY(bool deviceStreaming READ deviceStreaming NOTIFY deviceStreamingChanged)
  Q_PROPERTY(bool boardReady READ boardReady NOTIFY boardReadyChanged)
  Q_PROPERTY(QString firmwareVersion READ firmwareVersion NOTIFY firmwareVersionChanged)
  Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
  Q_PROPERTY(bool streamConnected READ streamConnected NOTIFY streamConnectedChanged)

public:
  explicit SessionManager(AuthManager *authManager,
                          DeviceManager *deviceManager,
                          StreamUploader *streamUploader,
                          BridgeDeviceClient *bridgeDeviceClient,
                          TokenStore *tokenStore,
                          DeepLinkHandler *deepLinkHandler,
                          const QVariantMap &bridgeRuntimeSettings,
                          QObject *parent = nullptr);

  QString state() const;
  QString uiState() const;
  QString statusMessage() const;
  bool authenticated() const;
  QStringList availablePorts() const;
  QString selectedPort() const;
  bool deviceConnected() const;
  bool deviceStreaming() const;
  bool boardReady() const;
  QString firmwareVersion() const;
  QString connectionStatus() const;
  bool streamConnected() const;

  Q_INVOKABLE void beginLogin();
  Q_INVOKABLE void autoLoginIfReady();
  Q_INVOKABLE void logout();
  Q_INVOKABLE void refreshPorts();
  Q_INVOKABLE void connectDevice();
  Q_INVOKABLE void autoConnectDevice();
  Q_INVOKABLE void disconnectDevice();
  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();

  void setSelectedPort(const QString &portName);

signals:
  void stateChanged();
  void uiStateChanged();
  void statusMessageChanged();
  void authenticatedChanged();
  void availablePortsChanged();
  void selectedPortChanged();
  void deviceConnectedChanged();
  void deviceStreamingChanged();
  void boardReadyChanged();
  void firmwareVersionChanged();
  void connectionStatusChanged();
  void streamConnectedChanged();

private:
  void notifyUiStateChanged();
  void setState(const QString &state);
  void setStatusMessage(const QString &message);

private slots:
  void handleAuthenticatedForAutoConnect();

private:
  AuthManager *m_authManager;
  DeviceManager *m_deviceManager;
  StreamUploader *m_streamUploader;
  BridgeDeviceClient *m_bridgeDeviceClient;
  TokenStore *m_tokenStore;
  DeepLinkHandler *m_deepLinkHandler;
  QString m_state;
  QString m_statusMessage;
  QVariantMap m_bridgeApiSettings;
  bool m_autoConnectOnStartup;
  bool m_autoConnectStartupDone;
};

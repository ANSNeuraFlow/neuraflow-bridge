#pragma once

#include <QObject>
#include <QUrl>

class DeepLinkHandler : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString clientId READ clientId NOTIFY payloadChanged)
  Q_PROPERTY(QString redirectUri READ redirectUri NOTIFY payloadChanged)
  Q_PROPERTY(QString state READ state NOTIFY payloadChanged)
  Q_PROPERTY(bool hasConnectPayload READ hasConnectPayload NOTIFY payloadChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)

public:
  explicit DeepLinkHandler(QObject *parent = nullptr);

  void configureContract(const QString &protocol,
                         const QString &action,
                         const QString &clientIdKey,
                         const QString &redirectUriKey,
                         const QString &stateKey,
                         quint16 callbackPort);

  Q_INVOKABLE void processArguments(const QStringList &arguments);
  Q_INVOKABLE bool processUrl(const QString &urlString);
  Q_INVOKABLE QUrl buildAuthStartUrl(const QString &baseUrl, const QString &authStartPath) const;

  QString clientId() const;
  QString redirectUri() const;
  QString state() const;
  bool hasConnectPayload() const;
  QString lastError() const;

signals:
  void payloadChanged();
  void errorChanged();

private:
  bool isValidRedirectUri(const QString &redirectUri) const;
  bool processUrlInternal(const QUrl &url);
  void setError(const QString &message);

  QString m_protocol;
  QString m_action;
  QString m_clientIdKey;
  QString m_redirectUriKey;
  QString m_stateKey;
  quint16 m_callbackPort;
  QString m_clientId;
  QString m_redirectUri;
  QString m_state;
  QString m_lastError;
};

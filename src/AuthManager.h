#pragma once

#include <QObject>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class CallbackServer;
class DeepLinkHandler;
class TokenStore;

class AuthManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString authState READ authState NOTIFY authStateChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)

public:
  explicit AuthManager(DeepLinkHandler *deepLinkHandler,
                       CallbackServer *callbackServer,
                       TokenStore *tokenStore,
                       QObject *parent = nullptr);

  QString authState() const;
  QString statusMessage() const;
  bool authenticated() const;

  void configureAuthConstraints(int codeTtlSeconds, const QStringList &allowedClientIds);

  Q_INVOKABLE bool beginLogin(const QString &webUrl, const QString &apiUrl, const QString &authStartPath, const QString &authTokenPath);
  Q_INVOKABLE void logout();

signals:
  void authStateChanged();
  void statusMessageChanged();
  void authenticatedChanged();
  void tokenReady(const QString &token);
  void authFailed(const QString &message);

private:
  void onCallbackReceived(const QString &code, const QString &state);
  void exchangeToken(const QString &code);
  void setAuthState(const QString &state);
  void setStatusMessage(const QString &message);

  DeepLinkHandler *m_deepLinkHandler;
  CallbackServer *m_callbackServer;
  TokenStore *m_tokenStore;
  QNetworkAccessManager *m_networkManager;
  QTimer *m_callbackTimeoutTimer;
  QNetworkReply *m_tokenReply;
  QString m_authState;
  QString m_statusMessage;
  QString m_expectedState;
  QString m_apiUrl;
  QString m_authTokenPath;
  QStringList m_allowedClientIds;
  int m_codeTtlSeconds;
};

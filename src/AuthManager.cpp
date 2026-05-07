#include "AuthManager.h"

#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>

#include "CallbackServer.h"
#include "DeepLinkHandler.h"
#include "TokenStore.h"

AuthManager::AuthManager(DeepLinkHandler *deepLinkHandler,
                         CallbackServer *callbackServer,
                         TokenStore *tokenStore,
                         QObject *parent)
    : QObject(parent),
      m_deepLinkHandler(deepLinkHandler),
      m_callbackServer(callbackServer),
      m_tokenStore(tokenStore),
      m_networkManager(new QNetworkAccessManager(this)),
      m_callbackTimeoutTimer(new QTimer(this)),
      m_tokenReply(nullptr),
      m_authState(QStringLiteral("idle"))
{
  m_codeTtlSeconds = 120;
  m_callbackTimeoutTimer->setSingleShot(true);

  connect(m_callbackTimeoutTimer, &QTimer::timeout, this, [this]()
          {
    const QString error = QStringLiteral("Authentication callback timed out");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    if (m_callbackServer)
    {
      m_callbackServer->stop();
    }
    emit authFailed(error); });

  connect(m_callbackServer, &CallbackServer::callbackReceived, this, &AuthManager::onCallbackReceived);
  connect(m_callbackServer, &CallbackServer::callbackError, this, [this](const QString &error)
          {
        setStatusMessage(error);
        setAuthState(QStringLiteral("error"));
        emit authFailed(error); });

  if (m_tokenStore)
  {
    connect(m_tokenStore, &TokenStore::tokenChanged, this, [this]()
            { emit authenticatedChanged(); });
  }
}

QString AuthManager::authState() const
{
  return m_authState;
}

QString AuthManager::statusMessage() const
{
  return m_statusMessage;
}

bool AuthManager::authenticated() const
{
  return m_tokenStore && m_tokenStore->hasValidToken();
}

void AuthManager::configureAuthConstraints(int codeTtlSeconds, const QStringList &allowedClientIds)
{
  m_codeTtlSeconds = qMax(30, codeTtlSeconds);
  m_allowedClientIds = allowedClientIds;
  m_allowedClientIds.removeAll(QString());
}

bool AuthManager::beginLogin(const QString &webUrl, const QString &apiUrl, const QString &authStartPath, const QString &authTokenPath)
{
  if (!m_deepLinkHandler || !m_deepLinkHandler->hasConnectPayload())
  {
    const QString error = QStringLiteral("Missing deep-link connect payload");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    emit authFailed(error);
    return false;
  }

  if (!m_allowedClientIds.isEmpty() && !m_allowedClientIds.contains(m_deepLinkHandler->clientId()))
  {
    const QString error = QStringLiteral("Deep-link clientId is not allowed");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    emit authFailed(error);
    return false;
  }

  m_apiUrl = apiUrl;
  m_authTokenPath = authTokenPath;
  m_expectedState = m_deepLinkHandler->state();

  if (!m_callbackServer->start())
  {
    const QString error = QStringLiteral("Unable to start localhost callback server");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    emit authFailed(error);
    return false;
  }

  const QUrl authUrl = m_deepLinkHandler->buildAuthStartUrl(webUrl, authStartPath);
  if (!authUrl.isValid())
  {
    const QString error = QStringLiteral("Cannot build valid auth URL");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    emit authFailed(error);
    return false;
  }

  setStatusMessage(QStringLiteral("Opened browser for bridge authentication"));
  setAuthState(QStringLiteral("waiting_callback"));
  const bool opened = QDesktopServices::openUrl(authUrl);
  if (!opened)
  {
    const QString error = QStringLiteral("Failed to open browser for authentication");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    emit authFailed(error);
    return false;
  }

  m_callbackTimeoutTimer->start(m_codeTtlSeconds * 1000);
  return true;
}

void AuthManager::logout()
{
  if (m_callbackTimeoutTimer->isActive())
  {
    m_callbackTimeoutTimer->stop();
  }

  if (m_tokenReply)
  {
    m_tokenReply->abort();
    m_tokenReply->deleteLater();
    m_tokenReply = nullptr;
  }

  if (m_tokenStore)
  {
    m_tokenStore->clearToken();
  }
  setAuthState(QStringLiteral("idle"));
  setStatusMessage(QStringLiteral("Logged out"));
  emit authenticatedChanged();
}

void AuthManager::onCallbackReceived(const QString &code, const QString &state)
{
  m_callbackTimeoutTimer->stop();

  if (state != m_expectedState)
  {
    const QString error = QStringLiteral("State mismatch in callback");
    setStatusMessage(error);
    setAuthState(QStringLiteral("error"));
    emit authFailed(error);
    return;
  }

  setAuthState(QStringLiteral("exchanging_token"));
  setStatusMessage(QStringLiteral("Exchanging auth code for bridge token"));
  if (m_callbackServer)
  {
    m_callbackServer->stop();
  }
  exchangeToken(code);
}

void AuthManager::exchangeToken(const QString &code)
{
  QUrl url(m_apiUrl);
  QString path = m_authTokenPath;
  if (!path.startsWith('/'))
  {
    path.prepend('/');
  }
  url.setPath(path);

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

  QJsonObject payload;
  payload.insert(QStringLiteral("code"), code);
  payload.insert(QStringLiteral("clientId"), m_deepLinkHandler->clientId());

  if (m_tokenReply)
  {
    m_tokenReply->abort();
    m_tokenReply->deleteLater();
    m_tokenReply = nullptr;
  }

  m_tokenReply = m_networkManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
  QPointer<QNetworkReply> guardedReply(m_tokenReply);
  connect(m_tokenReply, &QNetworkReply::finished, this, [this, guardedReply]()
          {
        if (!guardedReply)
        {
            return;
        }

        QNetworkReply *reply = guardedReply;
        m_tokenReply = nullptr;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            const QString error = QStringLiteral("Token exchange failed: %1").arg(reply->errorString());
            setStatusMessage(error);
            setAuthState(QStringLiteral("error"));
            emit authFailed(error);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        {
            const QString error = QStringLiteral("Invalid token response from backend");
            setStatusMessage(error);
            setAuthState(QStringLiteral("error"));
            emit authFailed(error);
            return;
        }

        const QJsonObject response = doc.object();
        const QString token = response.value(QStringLiteral("access_token")).toString();
        const int expiresIn = response.value(QStringLiteral("expires_in")).toInt(0);
        if (token.isEmpty() || expiresIn <= 0)
        {
            const QString error = QStringLiteral("Token response missing required fields");
            setStatusMessage(error);
            setAuthState(QStringLiteral("error"));
            emit authFailed(error);
            return;
        }

        if (m_tokenStore)
        {
            m_tokenStore->saveToken(token, expiresIn);
        }

        setStatusMessage(QStringLiteral("Bridge token acquired"));
        setAuthState(QStringLiteral("authenticated"));
        emit authenticatedChanged();
        emit tokenReady(token); });
}

void AuthManager::setAuthState(const QString &state)
{
  if (m_authState == state)
  {
    return;
  }
  m_authState = state;
  emit authStateChanged();
}

void AuthManager::setStatusMessage(const QString &message)
{
  if (m_statusMessage == message)
  {
    return;
  }
  m_statusMessage = message;
  emit statusMessageChanged();
}

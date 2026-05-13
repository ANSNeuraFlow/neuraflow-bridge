#include "DeepLinkHandler.h"

#include <QUuid>
#include <QUrlQuery>

DeepLinkHandler::DeepLinkHandler(QObject *parent)
    : QObject(parent),
      m_protocol(QStringLiteral("cyton-bridge")),
      m_action(QStringLiteral("connect")),
      m_clientIdKey(QStringLiteral("clientId")),
      m_redirectUriKey(QStringLiteral("redirectUri")),
      m_stateKey(QStringLiteral("state")),
      m_callbackPort(8787)
{
}

void DeepLinkHandler::configureContract(const QString &protocol,
                                        const QString &action,
                                        const QString &clientIdKey,
                                        const QString &redirectUriKey,
                                        const QString &stateKey,
                                        quint16 callbackPort)
{
  m_protocol = protocol.trimmed().isEmpty() ? QStringLiteral("cyton-bridge") : protocol.trimmed();
  m_action = action.trimmed().isEmpty() ? QStringLiteral("connect") : action.trimmed();
  m_clientIdKey = clientIdKey.trimmed().isEmpty() ? QStringLiteral("clientId") : clientIdKey.trimmed();
  m_redirectUriKey = redirectUriKey.trimmed().isEmpty() ? QStringLiteral("redirectUri") : redirectUriKey.trimmed();
  m_stateKey = stateKey.trimmed().isEmpty() ? QStringLiteral("state") : stateKey.trimmed();
  m_callbackPort = callbackPort;
}

void DeepLinkHandler::processArguments(const QStringList &arguments)
{
  for (const QString &argument : arguments)
  {
    if (argument.startsWith(m_protocol + QStringLiteral("://"), Qt::CaseInsensitive))
    {
      processUrl(argument);
      return;
    }
  }
}

bool DeepLinkHandler::processUrl(const QString &urlString)
{
  const QUrl url(urlString);
  if (!url.isValid())
  {
    setError(QStringLiteral("Invalid deep-link URL"));
    return false;
  }
  return processUrlInternal(url);
}

void DeepLinkHandler::seedSelfInitiated(const QString &clientId)
{
  const QString trimmed = clientId.trimmed();
  if (trimmed.isEmpty())
  {
    setError(QStringLiteral("Self-initiated login requires a non-empty clientId"));
    return;
  }

  m_clientId = trimmed;
  m_redirectUri = QStringLiteral("http://localhost:%1/callback").arg(m_callbackPort);
  m_state = QUuid::createUuid().toString(QUuid::WithoutBraces);
  m_lastError.clear();

  emit payloadChanged();
  emit errorChanged();
}

QUrl DeepLinkHandler::buildAuthStartUrl(const QString &baseUrl, const QString &authStartPath) const
{
  QUrl url(baseUrl);
  QString path = authStartPath;
  if (!path.startsWith('/'))
  {
    path.prepend('/');
  }
  url.setPath(path);

  QUrlQuery query;
  query.addQueryItem(m_clientIdKey, m_clientId);
  query.addQueryItem(m_redirectUriKey, m_redirectUri);
  query.addQueryItem(m_stateKey, m_state);
  url.setQuery(query);
  return url;
}

QString DeepLinkHandler::clientId() const
{
  return m_clientId;
}

QString DeepLinkHandler::redirectUri() const
{
  return m_redirectUri;
}

QString DeepLinkHandler::state() const
{
  return m_state;
}

bool DeepLinkHandler::hasConnectPayload() const
{
  return !m_clientId.isEmpty() && !m_redirectUri.isEmpty() && !m_state.isEmpty();
}

QString DeepLinkHandler::lastError() const
{
  return m_lastError;
}

bool DeepLinkHandler::processUrlInternal(const QUrl &url)
{
  if (url.scheme().compare(m_protocol, Qt::CaseInsensitive) != 0)
  {
    setError(QStringLiteral("Unsupported URL scheme"));
    return false;
  }

  const QString action = url.host().isEmpty() ? url.path().mid(1) : url.host();
  if (action.compare(m_action, Qt::CaseInsensitive) != 0)
  {
    setError(QStringLiteral("Unsupported deep-link action"));
    return false;
  }

  const QUrlQuery query(url);
  const QString clientId = query.queryItemValue(m_clientIdKey, QUrl::FullyDecoded);
  const QString redirectUri = query.queryItemValue(m_redirectUriKey, QUrl::FullyDecoded);
  const QString state = query.queryItemValue(m_stateKey, QUrl::FullyDecoded);

  if (clientId.isEmpty() || redirectUri.isEmpty() || state.isEmpty())
  {
    setError(QStringLiteral("Missing deep-link payload fields"));
    return false;
  }

  if (!isValidRedirectUri(redirectUri))
  {
    setError(QStringLiteral("Redirect URI must target localhost callback endpoint"));
    return false;
  }

  m_clientId = clientId;
  m_redirectUri = redirectUri;
  m_state = state;
  m_lastError.clear();

  emit payloadChanged();
  emit errorChanged();
  return true;
}

void DeepLinkHandler::setError(const QString &message)
{
  m_lastError = message;
  emit errorChanged();
}

bool DeepLinkHandler::isValidRedirectUri(const QString &redirectUri) const
{
  const QUrl uri(redirectUri);
  if (!uri.isValid())
  {
    return false;
  }

  if (uri.scheme() != QStringLiteral("http"))
  {
    return false;
  }

  const QString host = uri.host().toLower();
  if (host != QStringLiteral("localhost") && host != QStringLiteral("127.0.0.1"))
  {
    return false;
  }

  if (uri.path() != QStringLiteral("/callback"))
  {
    return false;
  }

  if (uri.port() <= 0)
  {
    return false;
  }

  return static_cast<quint16>(uri.port()) == m_callbackPort;
}

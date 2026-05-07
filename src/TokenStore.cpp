#include "TokenStore.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QSettings>
#include <QSysInfo>

static const char *kTokenKey = "BridgeRuntime/token";
static const char *kTokenExpiryKey = "BridgeRuntime/tokenExpiry";

TokenStore::TokenStore(QSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
  load();
}

void TokenStore::saveToken(const QString &token, int expiresInSeconds)
{
  m_token = token;
  m_expiry = QDateTime::currentDateTimeUtc().addSecs(expiresInSeconds);

  if (m_settings)
  {
    m_settings->setValue(kTokenKey, encryptToken(m_token));
    m_settings->setValue(kTokenExpiryKey, m_expiry);
    m_settings->sync();
  }

  emit tokenChanged();
}

void TokenStore::clearToken()
{
  m_token.clear();
  m_expiry = QDateTime();

  if (m_settings)
  {
    m_settings->remove(kTokenKey);
    m_settings->remove(kTokenExpiryKey);
    m_settings->sync();
  }

  emit tokenChanged();
}

bool TokenStore::hasValidToken() const
{
  return !m_token.isEmpty() && m_expiry.isValid() && m_expiry > QDateTime::currentDateTimeUtc();
}

QString TokenStore::token() const
{
  return m_token;
}

QDateTime TokenStore::expiry() const
{
  return m_expiry;
}

void TokenStore::load()
{
  if (!m_settings)
  {
    return;
  }

  m_token = m_settings->value(kTokenKey).toString();
  m_token = decryptToken(m_token);
  m_expiry = m_settings->value(kTokenExpiryKey).toDateTime();

  if (!hasValidToken())
  {
    m_token.clear();
    m_expiry = QDateTime();
  }

  emit tokenChanged();
}

QByteArray TokenStore::machineBoundKey() const
{
  const QString seed = QStringLiteral("%1|%2|%3").arg(
      QCoreApplication::applicationName(),
      QCoreApplication::organizationName(),
      QSysInfo::machineUniqueId().isEmpty()
          ? QString::fromUtf8(QSysInfo::bootUniqueId())
          : QString::fromUtf8(QSysInfo::machineUniqueId()));
  return QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha256);
}

QString TokenStore::encryptToken(const QString &plainToken) const
{
  if (plainToken.isEmpty())
  {
    return QString();
  }

  const QByteArray key = machineBoundKey();
  QByteArray tokenBytes = plainToken.toUtf8();
  for (int i = 0; i < tokenBytes.size(); ++i)
  {
    tokenBytes[i] = tokenBytes[i] ^ key.at(i % key.size());
  }
  return QString::fromLatin1(tokenBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString TokenStore::decryptToken(const QString &cipherToken) const
{
  if (cipherToken.isEmpty())
  {
    return QString();
  }

  const QByteArray decoded = QByteArray::fromBase64(cipherToken.toLatin1(), QByteArray::Base64UrlEncoding);
  if (decoded.isEmpty())
  {
    return QString();
  }

  const QByteArray key = machineBoundKey();
  QByteArray plain = decoded;
  for (int i = 0; i < plain.size(); ++i)
  {
    plain[i] = plain[i] ^ key.at(i % key.size());
  }
  return QString::fromUtf8(plain);
}

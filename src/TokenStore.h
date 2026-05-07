#pragma once

#include <QObject>
#include <QDateTime>

class QSettings;

class TokenStore : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool hasValidToken READ hasValidToken NOTIFY tokenChanged)
  Q_PROPERTY(QString token READ token NOTIFY tokenChanged)
  Q_PROPERTY(QDateTime expiry READ expiry NOTIFY tokenChanged)

public:
  explicit TokenStore(QSettings *settings, QObject *parent = nullptr);

  Q_INVOKABLE void saveToken(const QString &token, int expiresInSeconds);
  Q_INVOKABLE void clearToken();
  Q_INVOKABLE bool hasValidToken() const;

  QString token() const;
  QDateTime expiry() const;

signals:
  void tokenChanged();

private:
  QByteArray machineBoundKey() const;
  QString encryptToken(const QString &plainToken) const;
  QString decryptToken(const QString &cipherToken) const;
  void load();

  QSettings *m_settings;
  QString m_token;
  QDateTime m_expiry;
};

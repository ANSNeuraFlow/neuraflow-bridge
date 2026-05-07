#pragma once

#include <QObject>
#include <QStringList>

class QNetworkAccessManager;

class BridgeDeviceClient : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString deviceId READ deviceId NOTIFY deviceIdChanged)

public:
  explicit BridgeDeviceClient(QObject *parent = nullptr);

  QString deviceId() const;

  Q_INVOKABLE void registerDevice(const QString &baseUrl,
                                  const QString &devicesPath,
                                  const QString &token,
                                  const QString &deviceName,
                                  const QString &platform,
                                  const QString &version);
  Q_INVOKABLE void listDevices(const QString &baseUrl,
                               const QString &devicesPath,
                               const QString &token);

signals:
  void deviceIdChanged();
  void registerSucceeded(const QString &deviceId);
  void listSucceeded(const QStringList &deviceIds);
  void registerFailed(const QString &message);

private:
  QNetworkAccessManager *m_networkManager;
  QString m_deviceId;
};

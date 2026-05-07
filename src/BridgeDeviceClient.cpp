#include "BridgeDeviceClient.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace
{
  QUrl buildEndpointUrl(const QString &baseUrl, const QString &pathValue)
  {
    QUrl url(baseUrl);
    QString path = pathValue;
    if (!path.startsWith('/'))
    {
      path.prepend('/');
    }
    url.setPath(path);
    return url;
  }
}

BridgeDeviceClient::BridgeDeviceClient(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this))
{
}

QString BridgeDeviceClient::deviceId() const
{
  return m_deviceId;
}

void BridgeDeviceClient::registerDevice(const QString &baseUrl,
                                        const QString &devicesPath,
                                        const QString &token,
                                        const QString &deviceName,
                                        const QString &platform,
                                        const QString &version)
{
  if (token.isEmpty())
  {
    emit registerFailed(QStringLiteral("Missing bridge token"));
    return;
  }

  const QUrl url = buildEndpointUrl(baseUrl, devicesPath);
  if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty())
  {
    emit registerFailed(QStringLiteral("Invalid bridge devices endpoint"));
    return;
  }

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
  request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(token).toUtf8());

  QJsonObject payload;
  payload.insert(QStringLiteral("deviceName"), deviceName);
  payload.insert(QStringLiteral("platform"), platform);
  payload.insert(QStringLiteral("version"), version);

  QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
  connect(reply, &QNetworkReply::finished, this, [this, reply]()
          {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
          const QByteArray body = reply->readAll();
          emit registerFailed(QStringLiteral("Device registration failed: %1 (%2)").arg(reply->errorString(), QString::fromUtf8(body.left(256))));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        {
            emit registerFailed(QStringLiteral("Invalid device registration response"));
            return;
        }

        const QString id = doc.object().value(QStringLiteral("deviceId")).toString();
        if (id.isEmpty())
        {
            emit registerFailed(QStringLiteral("Device registration response missing deviceId"));
            return;
        }

        m_deviceId = id;
        emit deviceIdChanged();
        emit registerSucceeded(id); });
}

void BridgeDeviceClient::listDevices(const QString &baseUrl,
                                     const QString &devicesPath,
                                     const QString &token)
{
  if (token.isEmpty())
  {
    emit registerFailed(QStringLiteral("Missing bridge token"));
    return;
  }

  const QUrl url = buildEndpointUrl(baseUrl, devicesPath);
  if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty())
  {
    emit registerFailed(QStringLiteral("Invalid bridge devices endpoint"));
    return;
  }

  QNetworkRequest request(url);
  request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(token).toUtf8());

  QNetworkReply *reply = m_networkManager->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply]()
          {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
      emit registerFailed(QStringLiteral("List devices failed: %1").arg(reply->errorString()));
      return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray())
    {
      emit registerFailed(QStringLiteral("Invalid list devices response"));
      return;
    }

    QStringList ids;
    const QJsonArray arr = doc.array();
    for (const QJsonValue &value : arr)
    {
      if (!value.isObject())
      {
        continue;
      }
      const QString id = value.toObject().value(QStringLiteral("deviceId")).toString();
      if (!id.isEmpty())
      {
        ids.append(id);
      }
    }

    emit listSucceeded(ids); });
}

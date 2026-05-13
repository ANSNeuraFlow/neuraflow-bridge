#include "BackendController.h"

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <QtGlobal>

namespace
{
  constexpr int kHeartbeatIntervalMs = 30000;
}

BackendController::BackendController(QObject *parent)
    : QObject(parent),
      m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
      m_reconnectTimer(new QTimer(this)),
      m_heartbeatTimer(new QTimer(this)),
      m_connected(false),
      m_shouldReconnect(false),
      m_reconnectAttempts(0)
{
  m_reconnectTimer->setSingleShot(true);
  connect(m_reconnectTimer, &QTimer::timeout, this, [this]()
          {
            if (m_shouldReconnect)
            {
              connectToBackend(m_wsUrl, m_token);
            }
          });

  m_heartbeatTimer->setInterval(kHeartbeatIntervalMs);
  connect(m_heartbeatTimer, &QTimer::timeout, this, &BackendController::sendHeartbeat);

  connect(m_socket, &QWebSocket::connected, this, &BackendController::onSocketConnected);
  connect(m_socket, &QWebSocket::errorOccurred, this, &BackendController::onSocketError);
  connect(m_socket, &QWebSocket::disconnected, this, &BackendController::onSocketDisconnected);
  connect(m_socket, &QWebSocket::textMessageReceived, this, &BackendController::onTextMessageReceived);
}

bool BackendController::connected() const
{
  return m_connected;
}

void BackendController::connectToBackend(const QString &wsUrl, const QString &token)
{
  if (wsUrl.isEmpty())
  {
    emit statusMessage(QStringLiteral("Control WebSocket URL is not configured"));
    return;
  }

  const bool targetChanged = (wsUrl != m_wsUrl || token != m_token);
  m_wsUrl = wsUrl;
  m_token = token;
  m_shouldReconnect = true;

  QNetworkRequest request{QUrl(wsUrl)};
  if (!token.isEmpty())
  {
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(token).toUtf8());
  }

  if (!targetChanged &&
      (m_socket->state() == QAbstractSocket::ConnectedState ||
       m_socket->state() == QAbstractSocket::ConnectingState))
  {
    return;
  }

  if (m_socket->state() != QAbstractSocket::UnconnectedState)
  {
    m_socket->abort();
  }

  m_socket->open(request);
}

void BackendController::disconnectFromBackend()
{
  m_shouldReconnect = false;
  m_reconnectTimer->stop();
  m_heartbeatTimer->stop();

  if (m_socket->state() != QAbstractSocket::UnconnectedState)
  {
    m_socket->abort();
  }
}

void BackendController::sendStreamingStatus(bool streaming)
{
  if (!m_connected)
  {
    return;
  }

  QJsonObject root;
  root.insert(QStringLiteral("type"), QStringLiteral("status"));
  root.insert(QStringLiteral("streaming"), streaming);
  m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void BackendController::onSocketConnected()
{
  const bool wasConnected = m_connected;
  m_connected = true;
  m_reconnectAttempts = 0;
  if (!wasConnected)
  {
    emit connectedChanged();
  }
  emit statusMessage(QStringLiteral("Backend control connected"));
  m_heartbeatTimer->start();
}

void BackendController::onSocketDisconnected()
{
  m_heartbeatTimer->stop();
  const bool wasConnected = m_connected;
  m_connected = false;
  if (wasConnected)
  {
    emit connectedChanged();
  }
  emit statusMessage(QStringLiteral("Backend control disconnected"));
  if (m_shouldReconnect)
  {
    scheduleReconnect();
  }
}

void BackendController::onSocketError()
{
  const QAbstractSocket::SocketError code = m_socket->error();
  const QString err = m_socket->errorString();
  qWarning() << "Backend control WebSocket error:" << static_cast<int>(code) << err
             << "url:" << m_wsUrl;
  emit statusMessage(QStringLiteral("Backend control: %1").arg(err));
  if (m_shouldReconnect)
  {
    scheduleReconnect();
  }
}

void BackendController::sendHeartbeat()
{
  if (!m_connected)
  {
    return;
  }
  QJsonObject root;
  root.insert(QStringLiteral("type"), QStringLiteral("heartbeat"));
  m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void BackendController::onTextMessageReceived(const QString &message)
{
  parseAndDispatchMessage(message);
}

void BackendController::parseAndDispatchMessage(const QString &message)
{
  const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
  if (!doc.isObject())
  {
    return;
  }
  const QJsonObject obj = doc.object();
  const QString type = obj.value(QStringLiteral("type")).toString();
  if (type == QStringLiteral("connected"))
  {
    return;
  }
  if (type == QStringLiteral("command"))
  {
    const QString action = obj.value(QStringLiteral("action")).toString();
    if (action == QStringLiteral("start_streaming"))
    {
      emit startStreamingRequested();
    }
    else if (action == QStringLiteral("stop_streaming"))
    {
      emit stopStreamingRequested();
    }
    else if (action == QStringLiteral("send_marker"))
    {
      const QString marker = obj.value(QStringLiteral("marker")).toString();
      if (!marker.isEmpty())
      {
        emit markerReceived(marker);
      }
    }
  }
}

void BackendController::scheduleReconnect()
{
  if (!m_shouldReconnect || m_wsUrl.isEmpty())
  {
    return;
  }

  if (m_reconnectTimer->isActive())
  {
    return;
  }

  m_reconnectAttempts++;
  const int delayMs = qMin(30000, 1000 * m_reconnectAttempts);
  emit statusMessage(QStringLiteral("Reconnecting backend control in %1 ms").arg(delayMs));
  m_reconnectTimer->start(delayMs);
}

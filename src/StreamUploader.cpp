#include "StreamUploader.h"

#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <QtGlobal>

StreamUploader::StreamUploader(QObject *parent)
    : QObject(parent),
      m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
      m_reconnectTimer(new QTimer(this)),
      m_maxQueueSize(1024),
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
    } });

  connect(m_socket, &QWebSocket::connected, this, [this]()
          {
        m_connected = true;
        m_reconnectAttempts = 0;
        emit connectedChanged();
        emit statusMessage(QStringLiteral("Stream uploader connected"));
        flushQueue(); });

  connect(m_socket, &QWebSocket::disconnected, this, [this]()
          {
        const bool wasConnected = m_connected;
        m_connected = false;
        if (wasConnected)
        {
            emit connectedChanged();
        }
        emit statusMessage(QStringLiteral("Stream uploader disconnected"));
        if (m_shouldReconnect)
        {
          scheduleReconnect();
        } });

  connect(m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError)
        {
            emit statusMessage(QStringLiteral("Stream uploader socket error"));
            if (m_shouldReconnect)
            {
              scheduleReconnect();
            } });
}

bool StreamUploader::connected() const
{
  return m_connected;
}

int StreamUploader::queuedFrames() const
{
  return m_queue.size();
}

void StreamUploader::connectToBackend(const QString &wsUrl, const QString &token)
{
  if (wsUrl.isEmpty())
  {
    emit statusMessage(QStringLiteral("Stream URL is not configured"));
    return;
  }

  m_wsUrl = wsUrl;
  m_token = token;
  m_shouldReconnect = true;

  QNetworkRequest request{QUrl(wsUrl)};
  if (!token.isEmpty())
  {
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(token).toUtf8());
  }

  if (m_socket->state() == QAbstractSocket::ConnectingState || m_socket->state() == QAbstractSocket::ConnectedState)
  {
    return;
  }

  m_socket->open(request);
}

void StreamUploader::disconnectFromBackend()
{
  m_shouldReconnect = false;
  m_reconnectTimer->stop();

  if (m_socket->state() == QAbstractSocket::ConnectedState)
  {
    m_socket->close();
  }
}

void StreamUploader::enqueueFrame(const QByteArray &frame)
{
  if (m_connected)
  {
    m_socket->sendBinaryMessage(frame);
    return;
  }

  if (m_queue.size() >= m_maxQueueSize)
  {
    m_queue.dequeue();
    emit statusMessage(QStringLiteral("Frame queue full; dropping oldest frame"));
  }
  m_queue.enqueue(frame);
  emit queueChanged();
}

void StreamUploader::flushQueue()
{
  while (m_connected && !m_queue.isEmpty())
  {
    m_socket->sendBinaryMessage(m_queue.dequeue());
  }
  emit queueChanged();
}

void StreamUploader::scheduleReconnect()
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
  emit statusMessage(QStringLiteral("Reconnecting stream uploader in %1 ms").arg(delayMs));
  m_reconnectTimer->start(delayMs);
}

#pragma once

#include <QObject>
#include <QQueue>

class QWebSocket;
class QTimer;

class StreamUploader : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
  Q_PROPERTY(int queuedFrames READ queuedFrames NOTIFY queueChanged)

public:
  explicit StreamUploader(QObject *parent = nullptr);

  bool connected() const;
  int queuedFrames() const;

  Q_INVOKABLE void connectToBackend(const QString &wsUrl, const QString &token);
  Q_INVOKABLE void disconnectFromBackend();
  Q_INVOKABLE void enqueueFrame(const QByteArray &frame);

signals:
  void connectedChanged();
  void queueChanged();
  void statusMessage(const QString &message);

private:
  void flushQueue();
  void scheduleReconnect();

  QWebSocket *m_socket;
  QTimer *m_reconnectTimer;
  QQueue<QByteArray> m_queue;
  int m_maxQueueSize;
  bool m_connected;
  QString m_wsUrl;
  QString m_token;
  bool m_shouldReconnect;
  int m_reconnectAttempts;
};

#pragma once

#include <QObject>
#include <QString>

class QTimer;
class QWebSocket;

/**
 * Raw WebSocket control channel to neuraflow-backend (/api/v1/bridge/control).
 * Receives start_streaming, stop_streaming, send_marker commands as JSON text frames.
 */
class BackendController : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

public:
  explicit BackendController(QObject *parent = nullptr);

  bool connected() const;

  Q_INVOKABLE void connectToBackend(const QString &wsUrl, const QString &token);
  Q_INVOKABLE void disconnectFromBackend();

public slots:
  void sendStreamingStatus(bool streaming);

signals:
  void connectedChanged();
  void startStreamingRequested();
  void stopStreamingRequested();
  void markerReceived(const QString &marker);
  void statusMessage(const QString &message);

private slots:
  void onTextMessageReceived(const QString &message);
  void onSocketConnected();
  void onSocketDisconnected();
  void onSocketError();
  void sendHeartbeat();

private:
  void scheduleReconnect();
  void parseAndDispatchMessage(const QString &message);

  QWebSocket *m_socket;
  QTimer *m_reconnectTimer;
  QTimer *m_heartbeatTimer;
  bool m_connected;
  QString m_wsUrl;
  QString m_token;
  bool m_shouldReconnect;
  int m_reconnectAttempts;
};

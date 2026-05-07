#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QUrl>

class QTcpServer;
class QTcpSocket;

class CallbackServer : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)
  Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)

public:
  explicit CallbackServer(QObject *parent = nullptr);
  ~CallbackServer() override;

  Q_INVOKABLE bool start();
  Q_INVOKABLE void stop();

  bool listening() const;
  quint16 port() const;
  void setPort(quint16 port);

signals:
  void callbackReceived(const QString &code, const QString &state);
  void callbackError(const QString &error);
  void listeningChanged();
  void portChanged();

private:
  void handleConnection();
  void handleSocketData(QTcpSocket *socket);
  void sendHttpResponse(QTcpSocket *socket, int status, const QString &body,
                        const QByteArray &contentType = QByteArrayLiteral("text/plain; charset=utf-8"));

  QTcpServer *m_server;
  QHash<QTcpSocket *, QByteArray> m_socketBuffers;
  quint16 m_port;
};

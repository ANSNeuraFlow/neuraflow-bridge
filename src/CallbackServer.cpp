#include "CallbackServer.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QUrlQuery>

namespace
{
  constexpr int kMaxRequestBytes = 8 * 1024;

  QString callbackSuccessHtml()
  {
    return QStringLiteral(
        R"HTML(<!DOCTYPE html>
        <html lang="en">
        <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <meta name="color-scheme" content="dark light">
        <title>NeuraFlow Bridge</title>
        <link rel="preconnect" href="https://fonts.googleapis.com">
        <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
        <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;600&display=swap" rel="stylesheet">
        <style>
        :root {
          --surface: 11, 13, 17;
          --surface-container: 20, 24, 29;
          --on-surface: 255, 255, 255;
          --on-surface-dim: 179, 179, 179;
          --accent: 59, 130, 246;
          --success: 132, 204, 22;
          --shadow: 0 0.1rem 0.3rem rgba(0, 0, 0, 0.35);
        }
        @media (prefers-color-scheme: light) {
          :root {
            --surface: 242, 242, 242;
            --surface-container: 233, 233, 233;
            --on-surface: 5, 3, 21;
            --on-surface-dim: 90, 90, 90;
            --accent: 0, 114, 245;
            --shadow: 0 0.1rem 0.2rem rgba(0, 0, 0, 0.08);
          }
        }
        *, *::before, *::after { box-sizing: border-box; }
        html { font-size: 62.5%; }
        body {
          margin: 0;
          min-height: 100vh;
          display: flex;
          align-items: center;
          justify-content: center;
          padding: 2.4rem 1.6rem;
          font-family: Roboto, system-ui, sans-serif;
          font-size: 1.6rem;
          line-height: 2.4rem;
          font-weight: 400;
          letter-spacing: -0.01rem;
          background: rgb(var(--surface));
          color: rgb(var(--on-surface));
          -webkit-font-smoothing: antialiased;
        }
        .card {
          width: 100%;
          max-width: 44rem;
          padding: 2.4rem;
          border-radius: 0.8rem;
          background: rgb(var(--surface-container));
          border: 0.1rem solid rgba(var(--on-surface), 0.08);
          box-shadow: var(--shadow);
        }
        .icon {
          width: 4.8rem;
          height: 4.8rem;
          margin-bottom: 1.6rem;
        }
        .icon .bg { fill: rgba(var(--success), 0.15); }
        .icon .ring { fill: none; stroke: rgb(var(--success)); stroke-width: 0.15rem; }
        .icon .mark { fill: none; stroke: rgb(var(--success)); stroke-width: 0.25rem; stroke-linecap: round; stroke-linejoin: round; }
        h1 {
          margin: 0 0 0.8rem;
          font-size: 2rem;
          line-height: 2.8rem;
          font-weight: 600;
          letter-spacing: -0.02rem;
        }
        p {
          margin: 0;
          color: rgb(var(--on-surface-dim));
          font-size: 1.6rem;
          line-height: 2.4rem;
        }
        .note {
          margin-top: 1.6rem;
          padding-top: 1.6rem;
          border-top: 0.1rem solid rgba(var(--on-surface), 0.08);
          font-size: 1.4rem;
          line-height: 2rem;
          color: rgb(var(--on-surface-dim));
        }
        .brand { color: rgb(var(--accent)); font-weight: 500; }
        </style>
        </head>
        <body>
        <main class="card">
        <svg class="icon" viewBox="0 0 48 48" aria-hidden="true">
        <circle class="bg" cx="24" cy="24" r="22"/>
        <circle class="ring" cx="24" cy="24" r="22"/>
        <path class="mark" d="M15 24l6 6 12-14"/>
        </svg>
        <h1>You're connected</h1>
        <p>Bridge sign-in finished successfully. You can close this tab and return to <span class="brand">NeuraFlow</span>.</p>
        <p class="note">If this window opened in your browser, it is safe to close. The desktop app will continue in the background.</p>
        </main>
        </body>
        </html>)HTML"
    );
  }
}

CallbackServer::CallbackServer(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_port(8787)
{
  connect(m_server, &QTcpServer::newConnection, this, &CallbackServer::handleConnection);
}

CallbackServer::~CallbackServer()
{
  stop();
}

bool CallbackServer::start()
{
  if (m_server->isListening())
  {
    return true;
  }

  const bool ok = m_server->listen(QHostAddress::LocalHost, m_port);
  emit listeningChanged();
  if (!ok)
  {
    emit callbackError(QStringLiteral("Failed to start callback server: %1").arg(m_server->errorString()));
  }
  return ok;
}

void CallbackServer::stop()
{
  if (!m_server->isListening())
  {
    return;
  }
  m_server->close();
  emit listeningChanged();
}

bool CallbackServer::listening() const
{
  return m_server->isListening();
}

quint16 CallbackServer::port() const
{
  return m_port;
}

void CallbackServer::setPort(quint16 port)
{
  if (m_port == port)
  {
    return;
  }
  m_port = port;
  emit portChanged();
}

void CallbackServer::handleConnection()
{
  while (m_server->hasPendingConnections())
  {
    QTcpSocket *socket = m_server->nextPendingConnection();
    m_socketBuffers.insert(socket, QByteArray());
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]()
            { handleSocketData(socket); });
    connect(socket, &QTcpSocket::disconnected, this, [this, socket]()
            {
      m_socketBuffers.remove(socket);
      socket->deleteLater(); });
  }
}

void CallbackServer::handleSocketData(QTcpSocket *socket)
{
  if (!m_socketBuffers.contains(socket))
  {
    m_socketBuffers.insert(socket, QByteArray());
  }

  QByteArray &buffer = m_socketBuffers[socket];
  buffer.append(socket->readAll());

  if (buffer.size() > kMaxRequestBytes)
  {
    sendHttpResponse(socket, 400, QStringLiteral("Request too large"));
    emit callbackError(QStringLiteral("Callback request too large"));
    m_socketBuffers.remove(socket);
    return;
  }

  const int headerEnd = buffer.indexOf("\r\n\r\n");
  if (headerEnd < 0)
  {
    return;
  }

  const QByteArray header = buffer.left(headerEnd);
  const QList<QByteArray> lines = header.split('\n');
  if (lines.isEmpty())
  {
    sendHttpResponse(socket, 400, QStringLiteral("Bad request"));
    m_socketBuffers.remove(socket);
    return;
  }

  const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
  if (requestLine.size() < 2)
  {
    sendHttpResponse(socket, 400, QStringLiteral("Bad request line"));
    m_socketBuffers.remove(socket);
    return;
  }

  const QString method = QString::fromUtf8(requestLine.at(0));
  const QString target = QString::fromUtf8(requestLine.at(1));

  if (method != QStringLiteral("GET"))
  {
    sendHttpResponse(socket, 405, QStringLiteral("Method not allowed"));
    m_socketBuffers.remove(socket);
    return;
  }

  const QUrl url(target);
  if (!url.isValid() || url.path() != QStringLiteral("/callback"))
  {
    sendHttpResponse(socket, 404, QStringLiteral("Not found"));
    m_socketBuffers.remove(socket);
    return;
  }

  const QUrlQuery query(url);
  const QString code = query.queryItemValue(QStringLiteral("code"));
  const QString state = query.queryItemValue(QStringLiteral("state"));

  if (code.isEmpty() || state.isEmpty())
  {
    sendHttpResponse(socket, 400, QStringLiteral("Missing code or state"));
    emit callbackError(QStringLiteral("Missing callback parameters"));
    m_socketBuffers.remove(socket);
    return;
  }

  sendHttpResponse(socket, 200, callbackSuccessHtml(),
                   QByteArrayLiteral("text/html; charset=utf-8"));
  m_socketBuffers.remove(socket);
  emit callbackReceived(code, state);
}

void CallbackServer::sendHttpResponse(QTcpSocket *socket, int status, const QString &body,
                                      const QByteArray &contentType)
{
  QString statusText = QStringLiteral("OK");
  if (status == 400)
  {
    statusText = QStringLiteral("Bad Request");
  }
  else if (status == 404)
  {
    statusText = QStringLiteral("Not Found");
  }
  else if (status == 405)
  {
    statusText = QStringLiteral("Method Not Allowed");
  }

  const QByteArray payload = body.toUtf8();
  QByteArray response;
  response += "HTTP/1.1 " + QByteArray::number(status) + " " + statusText.toUtf8() + "\r\n";
  response += "Content-Type: " + contentType + "\r\n";
  response += "Content-Length: " + QByteArray::number(payload.size()) + "\r\n";
  response += "Connection: close\r\n\r\n";
  response += payload;

  socket->write(response);
  socket->flush();
  socket->disconnectFromHost();
}

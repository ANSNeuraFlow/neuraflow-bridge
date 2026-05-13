#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlContext>
#include <QUrl>
#include <QSettings>
#include <QDir>
#include <QFontDatabase>
#include <QFont>
#include <QVariant>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMetaObject>
#include <memory>
#include "utils.h"
#include "TranslationManager.h"
#include "DeepLinkHandler.h"
#include "CallbackServer.h"
#include "TokenStore.h"
#include "AuthManager.h"
#include "DeviceManager.h"
#include "BridgeDeviceClient.h"
#include "StreamUploader.h"
#include "SessionManager.h"
#include "DataProcessor.h"
#include "TimeSeriesController.h"

namespace
{
  constexpr auto kSingleInstanceServerName = "neuraflow-bridge";

  QString findCustomSchemeDeepLink(const QStringList &arguments)
  {
    for (const QString &arg : arguments)
    {
      if (!arg.contains(QStringLiteral("://")))
      {
        continue;
      }
      if (arg.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
          arg.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive))
      {
        continue;
      }
      return arg;
    }
    return {};
  }
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

  QLocalSocket secondaryProbe;
  secondaryProbe.connectToServer(QString::fromUtf8(kSingleInstanceServerName));
  if (secondaryProbe.waitForConnected(200))
  {
    const QString urlToForward = findCustomSchemeDeepLink(QCoreApplication::arguments());
    if (!urlToForward.isEmpty())
    {
      secondaryProbe.write(urlToForward.toUtf8());
      secondaryProbe.flush();
      secondaryProbe.waitForBytesWritten(500);
    }
    return 0;
  }

  QString appDirPath = QCoreApplication::applicationDirPath();
  QString settingsFilePath = QDir(appDirPath).filePath("config.ini");
  qDebug() << "Settings file path: " << settingsFilePath;
  QSettings settings(settingsFilePath, QSettings::IniFormat);

  QVariantMap appSettings;

  QVariantMap appGroup;
  QString appName = getSetting<QString>(settings, "App/name", false, "NeuraFlow Bridge");
  appGroup.insert("name", appName);
  const bool debugMode =
      qEnvironmentVariableIsSet("DEBUG_MODE") ||
      getSetting<bool>(settings, "App/debugMode", false, false);
  appGroup.insert("debugMode", debugMode);
  appSettings.insert("app", appGroup);

  QVariantMap bridgeApiGroup;
  const QString webBaseUrl = getSetting<QString>(settings, "BridgeApi/webBaseUrl", false, QString());
  const QString webUrl = getSetting<QString>(
      settings,
      "BridgeApi/webUrl",
      false,
      webBaseUrl.isEmpty() ? QStringLiteral("http://localhost:3000") : webBaseUrl);
  const QString apiBaseUrl = getSetting<QString>(settings, "BridgeApi/apiBaseUrl", false, QString());
  const QString apiUrl = getSetting<QString>(
      settings,
      "BridgeApi/apiUrl",
      false,
      apiBaseUrl.isEmpty() ? QStringLiteral("http://localhost:4000") : apiBaseUrl);
  bridgeApiGroup.insert("webUrl", webUrl);
  bridgeApiGroup.insert("apiUrl", apiUrl);
  bridgeApiGroup.insert("authStartPath", getSetting<QString>(settings, "BridgeApi/authStartPath", false, "/bridge/auth/start"));
  bridgeApiGroup.insert("authTokenPath", getSetting<QString>(settings, "BridgeApi/authTokenPath", false, "/api/v1/bridge/auth/token"));
  bridgeApiGroup.insert("devicesPath", getSetting<QString>(settings, "BridgeApi/devicesPath", false, "/api/v1/bridge/devices"));
  bridgeApiGroup.insert("streamWsUrl", getSetting<QString>(settings, "BridgeApi/streamWsUrl", false, "ws://localhost:4000/api/v1/bridge/stream"));
  appSettings.insert("bridgeApi", bridgeApiGroup);

  QVariantMap bridgeAuthGroup;
  bridgeAuthGroup.insert("codeTtlSeconds", getSetting<int>(settings, "BridgeAuth/codeTtlSeconds", false, 120));
  bridgeAuthGroup.insert("tokenTtlSeconds", getSetting<int>(settings, "BridgeAuth/tokenTtlSeconds", false, 86400));
  bridgeAuthGroup.insert("allowedClientIds", getSetting<QString>(settings, "BridgeAuth/allowedClientIds", false, "cyton_bridge"));
  bridgeAuthGroup.insert("oneTimeCode", getSetting<bool>(settings, "BridgeAuth/oneTimeCode", false, true));
  bridgeAuthGroup.insert("selfClientId", getSetting<QString>(settings, "BridgeAuth/selfClientId", false, "cyton_bridge"));
  appSettings.insert("bridgeAuth", bridgeAuthGroup);

  QVariantMap bridgeRuntimeSettings = bridgeApiGroup;
  bridgeRuntimeSettings.insert(QStringLiteral("codeTtlSeconds"), bridgeAuthGroup.value(QStringLiteral("codeTtlSeconds")));
  bridgeRuntimeSettings.insert(QStringLiteral("allowedClientIds"), bridgeAuthGroup.value(QStringLiteral("allowedClientIds")));
  bridgeRuntimeSettings.insert(QStringLiteral("selfClientId"), bridgeAuthGroup.value(QStringLiteral("selfClientId")));
  bridgeRuntimeSettings.insert(QStringLiteral("autoConnectOnStartup"),
                               getSetting<bool>(settings, "Device/autoConnectOnStartup", false, false));

  QVariantMap deepLinkGroup;
  deepLinkGroup.insert("protocol", getSetting<QString>(settings, "DeepLink/protocol", false, "cyton-bridge"));
  deepLinkGroup.insert("action", getSetting<QString>(settings, "DeepLink/action", false, "connect"));
  deepLinkGroup.insert("clientIdKey", getSetting<QString>(settings, "DeepLink/clientIdKey", false, "clientId"));
  deepLinkGroup.insert("redirectUriKey", getSetting<QString>(settings, "DeepLink/redirectUriKey", false, "redirectUri"));
  deepLinkGroup.insert("stateKey", getSetting<QString>(settings, "DeepLink/stateKey", false, "state"));
  deepLinkGroup.insert("callbackPort", getSetting<int>(settings, "DeepLink/callbackPort", false, 8787));
  appSettings.insert("deepLink", deepLinkGroup);

  QCoreApplication::setApplicationName(appSettings["app"].toMap()["name"].toString());
  QCoreApplication::setOrganizationName("NeuraFlow");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);

  QCommandLineParser parser;
  parser.setApplicationDescription(appName);
  parser.addHelpOption();
  parser.addVersionOption();
  parser.process(app);

  QString fontPath = ":/fonts/roboto.ttf";
  int fontId = QFontDatabase::addApplicationFont(fontPath);
  if (fontId == -1)
  {
    qWarning() << "Failed to load font:" << fontPath;
    return -1;
  }

  QStringList families = QFontDatabase::applicationFontFamilies(fontId);
  if (families.isEmpty())
  {
    qWarning() << "No families found for font:" << fontPath;
    return -1;
  }

  QString family = families.at(0);
  qApp->setFont(QFont(family));
  qDebug() << "Loaded font:" << family;

  auto engine = std::make_unique<QQmlApplicationEngine>();
  QObject::connect(engine.get(), &QQmlApplicationEngine::quit, &app, &QCoreApplication::quit, Qt::QueuedConnection);
  QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreationFailed, &app, []()
                   { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  QString appDirUrl = QUrl::fromLocalFile(appDirPath).toString();
  engine->rootContext()->setContextProperty("appDirUrl", appDirUrl);

  engine->rootContext()->setContextProperty("appSettings", appSettings);

  TranslationManager translationManager(&app, engine.get());
  engine->rootContext()->setContextProperty("TranslationManager", &translationManager);

  DeepLinkHandler deepLinkHandler;
  const QVariantMap deepLinkSettings = appSettings["deepLink"].toMap();
  deepLinkHandler.configureContract(
      deepLinkSettings["protocol"].toString(),
      deepLinkSettings["action"].toString(),
      deepLinkSettings["clientIdKey"].toString(),
      deepLinkSettings["redirectUriKey"].toString(),
      deepLinkSettings["stateKey"].toString(),
      static_cast<quint16>(deepLinkSettings["callbackPort"].toInt()));
  deepLinkHandler.processArguments(app.arguments());

  CallbackServer callbackServer;
  callbackServer.setPort(static_cast<quint16>(deepLinkSettings["callbackPort"].toInt()));

  TokenStore tokenStore(&settings);
  AuthManager authManager(&deepLinkHandler, &callbackServer, &tokenStore);
  DeviceManager deviceManager;
  deviceManager.setDebugMode(debugMode);
  BridgeDeviceClient bridgeDeviceClient;
  StreamUploader streamUploader;
  SessionManager sessionManager(
      &authManager,
      &deviceManager,
      &streamUploader,
      &bridgeDeviceClient,
      &tokenStore,
      &deepLinkHandler,
      bridgeRuntimeSettings);

  DataProcessor dataProcessor;
  TimeSeriesController timeSeriesController(&dataProcessor);

  QObject::connect(
      &deviceManager,
      &DeviceManager::effectiveSampleRateHzChanged,
      &dataProcessor,
      [&deviceManager, &dataProcessor]()
      {
        dataProcessor.setSampleRateHz(deviceManager.effectiveSampleRateHz());
      });
  dataProcessor.setSampleRateHz(deviceManager.effectiveSampleRateHz());

  QObject::connect(&deviceManager, &DeviceManager::frameReady,
                   &dataProcessor, &DataProcessor::ingestFrame);

  engine->rootContext()->setContextProperty("SessionManager", &sessionManager);
  engine->rootContext()->setContextProperty("DataProcessor", &dataProcessor);
  engine->rootContext()->setContextProperty("TimeSeriesController", &timeSeriesController);

  engine->loadFromModule("NeuraFlowBridge", "Main");

  if (engine->rootObjects().isEmpty())
  {
    engine.reset();
    return -1;
  }

  QQuickWindow *window = qobject_cast<QQuickWindow *>(engine->rootObjects().constFirst());
  window->show();

  QLocalServer::removeServer(QString::fromUtf8(kSingleInstanceServerName));
  auto *instanceServer = new QLocalServer(&app);
  if (!instanceServer->listen(QString::fromUtf8(kSingleInstanceServerName)))
  {
    qWarning() << "Single-instance server listen failed:" << instanceServer->errorString();
  }
  QObject::connect(instanceServer, &QLocalServer::newConnection, instanceServer,
                   [instanceServer, &deepLinkHandler, window]()
                   {
                     auto *conn = instanceServer->nextPendingConnection();
                     if (!conn)
                     {
                       return;
                     }
                     QObject::connect(conn, &QLocalSocket::readyRead, conn,
                                      [conn, &deepLinkHandler, window]()
                                      {
                                        const QString url = QString::fromUtf8(conn->readAll()).trimmed();
                                        if (!url.isEmpty())
                                        {
                                          deepLinkHandler.processUrl(url);
                                        }
                                        if (window)
                                        {
                                          window->show();
                                          window->raise();
                                          window->requestActivate();
                                        }
                                        conn->deleteLater();
                                      });
                   });

  if (deepLinkHandler.hasConnectPayload())
  {
    QMetaObject::invokeMethod(&sessionManager, "autoLoginIfReady", Qt::QueuedConnection);
  }

  int ret = app.exec();

  engine.reset();

  qInfo() << "Exiting";

  return ret;
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <QQmlContext>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QFontDatabase>
#include <QFont>
#include <QVariant>
#include <QDebug>
#include <memory>
#include "utils.h"
#include "translation_manager.h"
#include "DeepLinkHandler.h"
#include "CallbackServer.h"
#include "TokenStore.h"
#include "AuthManager.h"
#include "DeviceManager.h"
#include "BridgeDeviceClient.h"
#include "StreamUploader.h"
#include "SessionManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QString appDirPath = QCoreApplication::applicationDirPath();
    QString settingsFilePath = QDir(appDirPath).filePath("config.ini");
    qDebug() << "Settings file path: " << settingsFilePath;
    QSettings settings(settingsFilePath, QSettings::IniFormat);

    QVariantMap appSettings;

    QVariantMap appGroup;
    QString appName = getSetting<QString>(settings, "App/name", false, "NeuraFlow Bridge");
    appGroup.insert("name", appName);
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
    appSettings.insert("bridgeAuth", bridgeAuthGroup);

    QVariantMap deepLinkGroup;
    deepLinkGroup.insert("protocol", getSetting<QString>(settings, "DeepLink/protocol", false, "cyton-bridge"));
    deepLinkGroup.insert("action", getSetting<QString>(settings, "DeepLink/action", false, "connect"));
    deepLinkGroup.insert("clientIdKey", getSetting<QString>(settings, "DeepLink/clientIdKey", false, "clientId"));
    deepLinkGroup.insert("redirectUriKey", getSetting<QString>(settings, "DeepLink/redirectUriKey", false, "redirectUri"));
    deepLinkGroup.insert("stateKey", getSetting<QString>(settings, "DeepLink/stateKey", false, "state"));
    deepLinkGroup.insert("callbackPort", getSetting<int>(settings, "DeepLink/callbackPort", false, 8787));
    appSettings.insert("deepLink", deepLinkGroup);

    // --- Application Info ---
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
    qApp->setFont(QFont(family)); // global default
    qDebug() << "Loaded font:" << family;

    auto engine = std::make_unique<QQmlApplicationEngine>();
    QObject::connect(engine.get(), &QQmlApplicationEngine::quit, &app, &QGuiApplication::quit, Qt::QueuedConnection);
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
    BridgeDeviceClient bridgeDeviceClient;
    StreamUploader streamUploader;
    SessionManager sessionManager(
        &authManager,
        &deviceManager,
        &streamUploader,
        &bridgeDeviceClient,
        &tokenStore,
        &deepLinkHandler);

    engine->rootContext()->setContextProperty("SessionManager", &sessionManager);

    engine->loadFromModule("NeuraFlowBridge", "Main");

    if (engine->rootObjects().isEmpty())
    {
        engine.reset();
        return -1;
    }

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine->rootObjects().constFirst());
    window->show();

    int ret = app.exec();

    // Destroy the engine while SessionManager (and deps) still exist: stack unwind would
    // otherwise delete SessionManager before the unique_ptr engine, and QML teardown would
    // evaluate bindings against a dangling context property.
    engine.reset();

    qInfo() << "Exiting";

    return ret;
}

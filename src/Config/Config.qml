pragma Singleton
import QtQuick

QtObject {
    readonly property string appName: appSettings.app.name

    readonly property var bridgeApi: appSettings.bridgeApi
    readonly property string bridgeApiWebUrl: appSettings.bridgeApi.webUrl
    readonly property string bridgeApiApiUrl: appSettings.bridgeApi.apiUrl
    readonly property string bridgeAuthStartPath: appSettings.bridgeApi.authStartPath
    readonly property string bridgeAuthTokenPath: appSettings.bridgeApi.authTokenPath
    readonly property string bridgeDevicesPath: appSettings.bridgeApi.devicesPath
    readonly property string bridgeStreamWsUrl: appSettings.bridgeApi.streamWsUrl
    readonly property string bridgeControlWsUrl: appSettings.bridgeApi.controlWsUrl

    readonly property var bridgeAuth: appSettings.bridgeAuth
    readonly property int bridgeCodeTtlSeconds: appSettings.bridgeAuth.codeTtlSeconds
    readonly property int bridgeTokenTtlSeconds: appSettings.bridgeAuth.tokenTtlSeconds
    readonly property string bridgeAllowedClientIds: appSettings.bridgeAuth.allowedClientIds
    readonly property bool bridgeOneTimeCode: appSettings.bridgeAuth.oneTimeCode

    readonly property var deepLink: appSettings.deepLink
    readonly property string deepLinkProtocol: appSettings.deepLink.protocol
    readonly property string deepLinkAction: appSettings.deepLink.action
    readonly property string deepLinkClientIdKey: appSettings.deepLink.clientIdKey
    readonly property string deepLinkRedirectUriKey: appSettings.deepLink.redirectUriKey
    readonly property string deepLinkStateKey: appSettings.deepLink.stateKey
    readonly property int deepLinkCallbackPort: appSettings.deepLink.callbackPort
}

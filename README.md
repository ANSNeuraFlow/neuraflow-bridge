# NeuraFlow Bridge

Desktop bridge app for Cyton/BrainFlow integration with NeuraFlow backend services.

## Phase 0 Contract Lock

### Backend endpoints

- `POST /api/v1/bridge/auth/start`
  - Requires logged-in user session
  - Body: `{ clientId, redirectUri, state }`
  - Response: `{ code, state, redirectUri }`
- `POST /api/v1/bridge/auth/token`
  - Public endpoint, throttled
  - Body: `{ code, clientId }`
  - Response: `{ access_token, token_type: "Bearer", expires_in, user }`
- `POST /api/v1/bridge/devices`
  - Requires `Authorization: Bearer <bridge_token>`
  - Body: `{ deviceName, platform, version }`
  - Response: `{ deviceId }`
- `GET /api/v1/bridge/devices`
  - Requires `Authorization: Bearer <bridge_token>`
  - Lists bridge devices for the current token user

### Auth constraints

- Auth code TTL: `120` seconds
- Bridge token TTL: `86400` seconds (24h)
- Allowed client IDs (default): `cyton_bridge`
- Auth codes are one-time use only

### Deep-link payload contract

Bridge connect payload keys are fixed to:

- `clientId`
- `redirectUri`
- `state`

State must be preserved end-to-end and validated on callback.

## Runtime Config

Contract defaults are configured in [config.ini](config.ini):

- `[BridgeApi]` `webUrl` (frontend / browser auth), `apiUrl` (HTTP API), endpoint paths, and `streamWsUrl`
- `[BridgeAuth]` TTL/allowed-clients/one-time-code constraints
- `[DeepLink]` protocol/action and canonical payload key names

These values are loaded at startup and exposed to QML through the Config singleton.

## i18n Notes

Translations are loaded from the current module namespace path:

- `:/qt/qml/NeuraFlowBridge/i18n/qml_<locale>.qm`

`TranslationManager` now handles locale variants (for example `pl_PL` -> `pl`) and removes previously installed translators before reloading.

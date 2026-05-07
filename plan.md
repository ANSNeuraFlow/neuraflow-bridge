## Plan: NeuraFlow Bridge MVP (Qt Quick + BrainFlow)

Build a production-ready desktop bridge that launches from web deep-link, authenticates through the implemented browser code flow, connects to Cyton via BrainFlow, and streams EEG frames to backend services guarded by bridge token auth. Keep QML thin, C++ services authoritative, and include tray/background mode plus a modern themed UI using provided dark color tokens.

Backend contract baseline is PR #16 (Feat/(NEU-41): bridge auth code/token flow + bridge device registration). This plan treats those endpoints and constraints as source-of-truth.

Frontend contract baseline is PR #14 (Feat/(NEU-42): bridge authorization page flow + post-login redirect handling). This plan treats the bridge auth page/middleware/query parsing behavior from that PR as source-of-truth.

**Steps**

1. Phase 0 - Contract lock and repo hardening
1. Lock backend contract to implemented endpoints:
1. `POST /api/v1/bridge/auth/start` (requires logged-in user session) with body `{ clientId, redirectUri, state }` returns `{ code, state, redirectUri }`.
1. `POST /api/v1/bridge/auth/token` (public, throttled) with body `{ code, clientId }` returns `{ access_token, token_type: "Bearer", expires_in, user }`.
1. `POST /api/v1/bridge/devices` (requires `Authorization: Bearer <bridge_token>`) with body `{ deviceName, platform, version }` returns `{ deviceId }`.
1. `GET /api/v1/bridge/devices` lists registered devices for current bridge token user.
1. Capture backend auth constraints in bridge config: code TTL 2 minutes, token TTL 24 hours, allowed client IDs (default `cyton_bridge`), one-time code semantics.
1. Confirm frontend deep-link payload fields and state handling: `clientId`, `redirectUri`, `state`.
1. Normalize project naming leftovers from legacy CCTV Puzzle references to NeuraFlow Bridge where they affect loading, translations, and deployment docs.
1. Fix translation resource namespace mismatch and ensure i18n loading works under current module name.

1. Phase 1 - Core bridge runtime (blocking)
1. Add AuthManager service in C++ to orchestrate login lifecycle: launch browser URL, hold nonce/state, receive callback code, call `POST /api/v1/bridge/auth/token`, emit auth state signals.
1. Add CallbackServer service in C++ (localhost only) to receive /callback with code and state, validate state, and route success/failure back to AuthManager.
1. Add TokenStore service for secure persistence and retrieval of bridge token and expiry (`expires_in` from token endpoint). Use OS keychain where feasible; fallback encrypted local storage only if keychain unavailable.
1. Add DeepLinkHandler service to parse app launch args and protocol URLs; support connect action payload from cyton-bridge://connect.
1. Ensure DeepLinkHandler emits web URL query params using frontend keys (`clientId`, `redirectUri`, `state`) and URL-encodes values.
1. Add DeviceManager service wrapping BrainFlow lifecycle: list ports, connect, configure, start stream, stop stream, disconnect, status signals.
1. Add BridgeDeviceClient service to call `POST /api/v1/bridge/devices` once bridge token is acquired and to optionally call `GET /api/v1/bridge/devices` for diagnostics.
1. Add StreamUploader service using the backend ingest contract selected for MVP. Reuse `Authorization: Bearer <bridge_token>` model and implement reconnect plus bounded queue with drop policy.
1. Add SessionManager orchestrator to coordinate AuthManager + DeviceManager + StreamUploader and expose explicit state machine to QML.

1. Phase 2 - QML UI (parallel after Phase 1 scaffolding)
1. Expand main UI from shell into bridge control panel: auth status, login button, port picker, backend endpoint, start/stop controls, stream health.
1. Add tray/background behaviors: minimize to tray, restore from tray, clear indicators for connected and streaming states.
1. Apply provided dark theme tokens as centralized design variables and map to controls for consistent modern look.
1. Keep all business logic out of QML; bind to SessionManager properties and invokables only.

1. Phase 3 - Platform integration and packaging (depends on Phases 1-2)
1. Register custom protocol handler cyton-bridge:// for Windows, macOS, and Linux packaging artifacts.
1. Ensure startup argument plumbing supports deep-link invocation and standard app launch.
1. Produce distributables per OS and include first-run checks for protocol registration and localhost callback readiness.

1. Phase 4 - Reliability and observability (parallel with late Phase 3)
1. Implement structured logs with correlation ids per session/auth attempt and upload lifecycle events.
1. Add watchdog health checks: auth validity, websocket liveness, device connectivity, stream throughput.
1. Add guarded retries with backoff for backend disconnects and dongle reconnects.
1. Add explicit user-facing error taxonomy in UI: auth failed, callback timeout, invalid/expired one-time code, unknown client id, bridge token expired, device unavailable, stream disconnected.

1. Phase 5 - Security hardening and release readiness
1. Enforce localhost callback URI validation (`http://localhost:<port>/callback`) and strict state/nonce checks.
1. Never log raw tokens or auth code values; redact sensitive fields.
1. Scope token usage to bridge stream actions only and respect expiry with refresh or re-auth.
1. Add threat-model checklist for protocol handler abuse and local callback interception.

**Relevant files**

- /home/mephew/University/projects/neuraflow-bridge/CMakeLists.txt - Add new C++ sources, required Qt modules (WebSockets, Network), BrainFlow linkage, platform packaging metadata.
- /home/mephew/University/projects/neuraflow-bridge/src/main.cpp - Wire service objects, protocol/deep-link arg handling, startup flow, tray integration hooks.
- /home/mephew/University/projects/neuraflow-bridge/src/Main.qml - Replace minimal shell with control dashboard shell and state bindings.
- /home/mephew/University/projects/neuraflow-bridge/src/Config/Config.qml - Extend runtime-config exposure for backend URL defaults and UI flags.
- /home/mephew/University/projects/neuraflow-bridge/config.ini - Add backend defaults, callback port defaults, optional debug toggles.
- /home/mephew/University/projects/neuraflow-bridge/src/translation_manager.cpp - Fix translation resource module path and keep i18n working after rename cleanup.
- /home/mephew/University/projects/neuraflow-bridge/src/translation_manager.h - Keep public API and add status signals only if needed by UI.
- /home/mephew/University/projects/neuraflow-bridge/src/qml_utils.h - Optional utility expansion for lightweight helpers that stay non-sensitive.
- /home/mephew/University/projects/neuraflow-bridge/src/qml_utils.cpp - Implement any added helper methods used by QML.
- /home/mephew/University/projects/neuraflow-bridge/src/utils.h - Reuse settings helpers for required/optional bridge settings.
- /home/mephew/University/projects/neuraflow-bridge/src/utils.cpp - Reuse group-loading for structured config sections.
- /home/mephew/University/projects/neuraflow-bridge/src/GameScreen.qml - Legacy scope; leave untouched for MVP unless explicitly repurposed.
- /home/mephew/University/projects/neuraflow-bridge/README.md - Replace legacy deployment instructions with bridge-specific install, deep-link, and troubleshooting docs.
- /home/mephew/University/projects/neuraflow-bridge/src/AuthManager.h - New file for auth flow state and browser-launch orchestration.
- /home/mephew/University/projects/neuraflow-bridge/src/AuthManager.cpp - New file for auth URL generation, token exchange, and lifecycle.
- /home/mephew/University/projects/neuraflow-bridge/src/CallbackServer.h - New file for localhost callback listener.
- /home/mephew/University/projects/neuraflow-bridge/src/CallbackServer.cpp - New file for callback parsing and response handling.
- /home/mephew/University/projects/neuraflow-bridge/src/TokenStore.h - New file for token persistence abstraction.
- /home/mephew/University/projects/neuraflow-bridge/src/TokenStore.cpp - New file for secure storage integration.
- /home/mephew/University/projects/neuraflow-bridge/src/DeepLinkHandler.h - New file for protocol payload parsing.
- /home/mephew/University/projects/neuraflow-bridge/src/DeepLinkHandler.cpp - New file for launch input normalization.
- /home/mephew/University/projects/neuraflow-bridge/src/DeviceManager.h - New file wrapping BrainFlow board/session lifecycle.
- /home/mephew/University/projects/neuraflow-bridge/src/DeviceManager.cpp - New file for serial port selection and stream start/stop.
- /home/mephew/University/projects/neuraflow-bridge/src/BridgeDeviceClient.h - New file for bridge device registration/list HTTP calls.
- /home/mephew/University/projects/neuraflow-bridge/src/BridgeDeviceClient.cpp - New file for `POST/GET /api/v1/bridge/devices` integration.
- /home/mephew/University/projects/neuraflow-bridge/src/StreamUploader.h - New file for backend socket client and queueing.
- /home/mephew/University/projects/neuraflow-bridge/src/StreamUploader.cpp - New file for binary frame serialization and control-plane messaging.
- /home/mephew/University/projects/neuraflow-bridge/src/SessionManager.h - New file for global bridge state machine exposed to QML.
- /home/mephew/University/projects/neuraflow-bridge/src/SessionManager.cpp - New file for orchestration and failure policy.
- /home/mephew/University/projects/neuraflow-bridge/src/qml/BridgeWindow.qml - New file for main bridge UI layout and bindings.
- /home/mephew/University/projects/neuraflow-bridge/src/qml/components/StatusBadge.qml - New reusable state indicator component.
- /home/mephew/University/projects/neuraflow-bridge/src/qml/components/PortPicker.qml - New serial-port selection component.
- /home/mephew/University/projects/neuraflow-bridge/src/qml/components/AuthPanel.qml - New login/connect state component.
- /home/mephew/University/projects/neuraflow-bridge/src/qml/theme/Theme.qml - New centralized color token mapping including provided dark palette.

**Verification**

1. Build and run on Linux/macOS/Windows local targets with CMake configure + compile success.
1. Deep-link launch test: clicking Connect Desktop Bridge in web app opens `/bridge/auth/start` with `clientId`, `redirectUri`, `state` query params; bridge completes round-trip with unchanged `state`.
1. Auth start/token test: browser flow yields localhost callback with `code` and unchanged `state`; bridge exchanges code via `POST /api/v1/bridge/auth/token`, stores token, reflects authenticated UI state.
1. Device test: enumerate ports, connect to Cyton dongle, start and stop BrainFlow session repeatedly without restart.
1. Device registration test: bridge registers device through `POST /api/v1/bridge/devices`, receives `deviceId`, and can fetch via `GET /api/v1/bridge/devices`.
1. Stream test: backend receives binary EEG frames at expected cadence and validates bridge bearer token.
1. Resilience test: unplug/replug dongle and transient backend disconnect while streaming, verify recovery behavior.
1. Tray/background test: closing window keeps bridge alive in tray (if enabled), restore works, stream continuity maintained.
1. Security test: invalid redirect URI, mismatched state, expired code, reused code, mismatched client id, and invalid/expired token are all rejected with clear UI errors.
1. i18n sanity test: language switching still functions and no resource path regressions.

**Decisions**

- Included scope: deep-link launch, browser code auth flow compatible with PR #16, device registration endpoint integration, serial picker, BrainFlow session control, backend streaming, background tray mode, modern QML theme integration.
- Included transport: binary EEG data over QWebSocket and JSON events for control/status.
- Excluded for MVP: advanced analytics dashboards, multi-device orchestration beyond backend registration/list, cloud model training orchestration, auto-update pipeline.
- Excluded unless later requested: reactivating legacy puzzle-specific modules in GameScreen.

**Further Considerations**

1. Backend payload schema recommendation: fixed-size binary frame with little-endian timestamp + 8 float32 channels + sequence id for loss detection.
2. Token storage recommendation: use native secure storage backend per OS before custom encryption fallback.
3. Operational recommendation: define backend ingest acknowledgment policy (fire-and-forget vs periodic ack) before tuning queue/drop strategy.
4. Integration recommendation: keep bridge auth client identifiers configurable to match `BRIDGE_ALLOWED_CLIENT_IDS` without desktop rebuild.

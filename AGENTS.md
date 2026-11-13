# AGENTS.md — Project Context for AI Coding

> Put this file at the repo root. AI coding tools (Cursor, Claude Code, Copilot, etc.) read it before writing code.

## 1. About the Project
- **Title:** Design and Build of a Multi-Node Wireless Sensor Network-Based Room Security and Intrusion Detection System with Encrypted Transmission Protection
- **Course:** Wireless Sensor Networks — Faculty of Computer Science, ITTS
- **Group 1:** Anathapindika Surja Putra, Galih Sukristyan Saputra, Mustofa Akhyar, Usamah Putra A
- **Goal:** Detect unauthorized room door openings, sound a local alarm, and send notifications to the internet. All inter-node communication **must be encrypted**.

## 2. Hardware (3× ESP32-C3)
| Node | Role | Components | Notes |
|---|---|---|---|
| `node-door` | Door sensor | Reed switch (magnetic door sensor), optional PIR | Send OPEN/CLOSE events to the gateway |
| `node-alarm` | Alarm actuator | Active buzzer and/or relay + LED | Receive ALARM_ON/OFF commands |
| `node-gateway` | Internet gateway | ESP32-C3 + Wi-Fi + SSD1306 OLED (I2C SDA 6 / SCL 7, `0x3C`) | ESP-NOW ↔ MQTT bridge, shows Wi-Fi/MQTT status and online node count |

Default pins (may be changed, put them in `config.h`):
- Reed switch → GPIO 4 (INPUT_PULLUP, LOW = closed); GPIO 3 is also read for the old button-board switch used in simulation.
- Buzzer → GPIO 5, plus GPIO 0/1 for old alarm-board wiring; Relay → GPIO 6, Status LED → GPIO 8 (onboard LED on some C3 boards)
- Gateway OLED → I2C SDA GPIO 6, SCL GPIO 7, address `0x3C`

## 3. Architecture & Topology
**Star** topology, with the gateway as the center (sink).

```
[node-door] --ESP-NOW (AES-128)--> [node-gateway] --Wi-Fi WPA2 + MQTT/TLS--> [MQTT Broker] --> Dashboard/Phone
                                         |
                                         +--ESP-NOW (AES-128)--> [node-alarm]
```

Flow:
1. Reed switch changes → `node-door` sends an event to the gateway over encrypted ESP-NOW.
2. The gateway publishes to MQTT, and when in ARMED mode sends `ALARM_ON` to `node-alarm`.
3. The user can arm/disarm/turn off the alarm over MQTT; the gateway forwards it to the relevant node.
4. Every node sends a periodic heartbeat so the gateway knows it is still alive.

## 4. Stack & Tools
- **Framework:** Arduino core for ESP32 (via PlatformIO in VS Code). `platformio.ini` uses `weactstudio_esp32c3coreboard`; switch to `esp32-c3-devkitm-1` for a DevKitM board.
- **Libraries:** `esp_now.h`, `WiFi.h`, `WiFiClientSecure.h`, `PubSubClient` (MQTT), `ArduinoJson`.
- **Broker:** public `broker.emqx.io` TCP TLS port `8883` for class use (no credentials); HiveMQ Cloud or local Mosquitto TLS also supported. The dashboard browser connects to the same broker at `wss://broker.emqx.io:8084/mqtt`.
- **Simulation (optional):** NS-3 for star-topology modeling — separate from the firmware.
- **Dashboard:** SvelteKit 3 + Svelte 5 (runes) + Tailwind CSS 4, `adapter-static` SPA, `mqtt.js` over WebSocket, Dexie 4 (IndexedDB).

## 5. Security (MANDATORY)
- ESP-NOW uses **PMK + LMK (AES-128 CCMP)**: `esp_now_set_pmk()` and `peer.encrypt = true` + `peer.lmk`.
- Wi-Fi WPA2/WPA3. MQTT over **TLS** (`WiFiClientSecure` + broker CA cert); never use plain port 1883.
- Payloads carry `seq` (sequence number) + `ts`; the gateway rejects messages with an old seq (simple anti-replay).
- **Never hardcode secrets in committed code.** All keys, SSID, password, and MQTT credentials go in `common/secrets.h` (listed in `.gitignore`); provide `common/secrets.example.h`.
- Whitelist peer MACs; ignore messages from unknown MACs.
- **Implementation status:** all three firmwares implement this design. Door and alarm use ESP-NOW unicast to the gateway with `peer.encrypt = true` (PMK + per-peer LMK), MAC whitelisting, and a monotonic `seq` check. Only the gateway joins Wi-Fi and connects to MQTT over TLS with a pinned CA; door/alarm never connect to the broker. Secrets live in `common/secrets.h` (gitignored) with `common/secrets.example.h` as template.

## 6. Message Format
ESP-NOW (binary struct, max 250 bytes):
```c
typedef struct __attribute__((packed)) {
  uint8_t  type;     // 1=DOOR_EVENT, 2=ALARM_CMD, 3=HEARTBEAT, 4=ACK
  uint8_t  nodeId;   // 1=door, 2=alarm, 0=gateway
  uint32_t seq;
  uint32_t ts;       // millis()
  uint8_t  value;    // DOOR/ALARM/HEARTBEAT: 0=CLOSED/OFF 1=OPEN/ON | ACK: state
} Packet;
```
`HEARTBEAT` carries the current door/alarm state so a lost event self-heals. `ACK` echoes the acknowledged `seq` (door) or the alarm's own `seq` with its current state (alarm ACK to gateway).

MQTT topics (prefix `kel1/wsn/`), JSON payloads:
| Topic | Direction | Example payload |
|---|---|---|
| `kel1/wsn/door/state` | gateway → broker | `{"state":"OPEN","seq":12,"ts":...}` (retained) |
| `kel1/wsn/alarm/state` | gateway → broker | `{"state":"ON"}` (retained) |
| `kel1/wsn/system/mode` | broker → gateway | `{"mode":"ARMED"}` / `DISARMED` |
| `kel1/wsn/alarm/cmd` | broker → gateway | `{"cmd":"OFF"}` |
| `kel1/wsn/node/<id>/status` | gateway → broker | `{"online":true,"rssi":-60}` (retained) |

Legacy contract (old `1003250005` firmware; the dashboard still consumes it for backward compatibility):
| Topic | Direction | Example payload |
|---|---|---|
| `1003250005/report` | device → broker | `[{"id":"sensor-AA:BB","name":"sensor","type":"sensor-temperature","value":27.5,"ts":1763000000}]` |
| `1003250005/event` | device → broker | `[{"id":"button-AA:BB","name":"button","type":"button","value":2,"ts":1763000000}]` |
| `1003250005/devices/<deviceId>` | broker → device | `[{"value":1}]` (0=OFF, 1=ON, 2=toggle) |

- `report` is the periodic state (every 10 s); `event` is a state change (door pressed → `value: 2`).
- Payloads are always JSON arrays of objects with `id`, `name`, `type`, `value`, `ts`.

## 7. Repository Structure
```
/common             (packet.h, config.h, espnow_common.h, secrets.example.h)
/deviceSensorDoor   (PlatformIO: door sensor node, ESP-NOW)
/deviceAlert        (PlatformIO: alarm/relay node, ESP-NOW)
/deviceHub          (PlatformIO: gateway ESP-NOW ↔ MQTT/TLS)
/src                (SvelteKit dashboard: MQTT + Dexie)
/docs               (topology, report, slides — optional)
AGENTS.md
README.md
```

## 8. Coding Rules
- Arduino C++, simple and readable for students; short comments in Bahasa Indonesia.
- **Non-blocking:** use `millis()`, avoid long `delay()` calls in `loop()`.
- Debounce the reed switch (±50 ms).
- Constants in `common/config.h`, secrets in `common/secrets.h` (template: `common/secrets.example.h`).
- Serial logs (115200) with prefixes `[DOOR]`, `[ALARM]`, `[GW]`.
- The gateway must auto-reconnect Wi-Fi and MQTT.
- ESP32-C3 note: ESP-NOW and Wi-Fi STA must use the **same channel** as the router — sensor nodes set their channel to the gateway AP's channel.
- Dashboard: Svelte 5 runes (`$state`, `$derived`, `$effect`), shared shadcn-svelte primitives, TypeScript strict, no direct DOM writes.

## 9. AI Task List
All firmware items below are implemented in the `device*/` projects; keep the packet/topic contract in §5–§6 stable when changing behavior.

1. Create `common/packet.h` and `common/config.h`.
2. `node-door` firmware: read the reed switch, debounce, send encrypted DOOR_EVENT + heartbeat every 30 seconds.
3. `node-alarm` firmware: receive ALARM_CMD, drive the buzzer/relay, send ACK + heartbeat.
4. `node-gateway` firmware: receive ESP-NOW, ARMED/DISARMED logic, publish over MQTT/TLS, subscribe to commands, forward them to `node-alarm`, mark a node offline when its heartbeat is missing for > 90 seconds.
5. `secrets.example.h` + README setup instructions (fill in MACs, keys, broker).
6. Dashboard `/` route: two tables (devices and events) backed by MQTT over WebSocket and Dexie; device online state derived from reports newer than 90 seconds.
7. (Optional) NS-3 star-topology script for 3 nodes to simulate delay/packet loss.

## 10. Definition of Done
- Door opened while ARMED → buzzer sounds in < 1 second and the message appears on the `door/state` topic.
- Disarm over MQTT → alarm turns off.
- ESP-NOW sniffing shows no plaintext payload; MQTT is TLS-only.
- Node unplugged → `online:false` status appears on MQTT.

## 11. Out of Scope
- Custom mobile app (an MQTT client/dashboard such as MQTT Explorer or the HiveMQ web client is enough).
- Database/cloud backend.

## 12. Dashboard
- Single route `/`, rendered client-side only (`ssr = false`), static build via `adapter-static`.
- **MQTT transport:** `mqtt.js` over WebSocket (browser). Subscribes to `<prefix>/#` for every configured prefix and consumes:
  - `<prefix>/door/state` — door `OPEN`/`CLOSED` state (gateway firmware).
  - `<prefix>/alarm/state` — alarm `ON`/`OFF` state (gateway firmware).
  - `<prefix>/system/mode` — gateway `ARMED`/`DISARMED` mode (broker → gateway).
  - `<prefix>/node/<id>/status` — node heartbeat/`online` flag (gateway firmware).
  - `<prefix>/report` / `<prefix>/event` — legacy firmware contract, still consumed.
- **Local storage:** Dexie 4 database `universe-network-sensor`.
  - `devices` — primary key `<id>:<type>`; fields `id`, `name`, `type`, `last_value`, `ts`, `last_seen_at`, `online`.
  - `events` — auto-increment id, indexed by `received_at`; newest 1000 rows retained.
- **Online rule:** a device with an explicit `online:false` status is offline; otherwise it is online while `now - last_seen_at <= 90 s`. The UI refreshes the age every second.
- **Tables:** devices (all rows) and events (latest 100, newest first).
- **Environment** (`APP_` prefix is required by `vite.config.ts`; set in `.env` and `.env.production`):
  - `APP_MQTT_HOST` — WebSocket broker URL, default `wss://broker.emqx.io:8084/mqtt`.
  - `APP_MQTT_USER`, `APP_MQTT_PASS` — optional credentials.
  - `APP_MQTT_TOPIC_PREFIXES` — comma-separated topic prefixes, default `1003250005,kel1/wsn` (legacy firmware + ESP-NOW gateway).
- **Structure:** `src/lib/data/db.ts` (Dexie), `src/lib/mqtt.ts` (connect + ingest), `src/lib/hooks/use-mqtt.svelte.ts` (connection status), `src/lib/hooks/use-live-query.svelte.ts` (Dexie live queries), `src/lib/types/device.ts` (records/status types).

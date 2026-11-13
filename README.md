# universe-network-sensor

ESP-NOW wireless sensor network for room security: a reed-switch door node, an alarm node, and a Wi-Fi gateway that bridges to MQTT over TLS. A SvelteKit dashboard reads the broker and stores devices/events in IndexedDB.

```
[node-door] --ESP-NOW AES-128--> [node-gateway] --Wi-Fi + MQTT/TLS--> broker --> dashboard
                                        |
                                        +--ESP-NOW AES-128--> [node-alarm]
```

## Verified configuration

This exact setup is flashed and running:

| Node | USB port | MAC | Role |
|---|---|---|---|
| Hub / gateway | `COM3` | `F0:F5:BD:FD:B5:D4` | Wi-Fi + MQTT TLS + OLED |
| Door | `COM4` | `88:56:A6:5C:8B:C0` | ESP-NOW only |
| Alarm | `COM5` | `88:56:A6:5C:80:20` | ESP-NOW only |

Verified on `kel1/wsn/#`:

- All three `node/*/status` topics publish `{"online":true}`.
- `door/state` publishes `OPEN`/`CLOSED`.
- `alarm/state` follows commands: `OFF -> ON -> OFF`.
- `ARMED` while the door is open turns the alarm ON; `DISARMED` turns it OFF.
- OLED reports `DEV : 2/2` and both nodes `ONLINE`.

The detailed investigation log is in [SUMMARY.md](SUMMARY.md).

## Repository

| Path | Contents |
|---|---|
| `common/` | shared `packet.h`, `config.h`, `espnow_common.h`, secrets template |
| `deviceSensorDoor/` | PlatformIO project: door node (ESP-NOW only) |
| `deviceAlert/` | PlatformIO project: alarm node (ESP-NOW only) |
| `deviceHub/` | PlatformIO project: gateway (ESP-NOW ↔ MQTT/TLS) and OLED |
| `src/` | SvelteKit dashboard (MQTT over WebSocket + Dexie) |
| `docs/` | optional report/slides |

## Setup

### 1. Secrets

```bash
cp common/secrets.example.h common/secrets.h
```

Fill `common/secrets.h`. It is gitignored; never commit it.

- `WIFI_SSID`, `WIFI_PASSWORD` — 2.4 GHz AP used by the gateway.
- `ESPNOW_CHANNEL` — the AP channel. It must match exactly (`6` for SmartHub in this build).
- `MQTT_HOST`, `MQTT_PORT` — default `broker.emqx.io` / `8883` (TCP TLS). Port `8084` is the WebSocket endpoint used by the browser dashboard, not by the ESP32/PubSubClient.
- `MQTT_USER`, `MQTT_PASSWORD` — leave empty for the public EMQX broker.
- `MQTT_CA_CERT` — already filled with DigiCert Global Root G2 for `broker.emqx.io`. Replace it if you switch brokers.
- `ESPNOW_PMK`, `ESPNOW_LMK_DOOR`, `ESPNOW_LMK_ALARM` — three independent 16-byte AES-128 keys. Generate them, e.g. `openssl rand -hex 16` three times, and write the bytes into the arrays.
- `MAC_GATEWAY`, `MAC_DOOR`, `MAC_ALARM` — STA MAC addresses.

### 2. Read the MAC addresses

Flash/upload any node with the template secrets and open the serial monitor at 115200. Each firmware prints its own MAC before ESP-NOW starts:

```
[DOOR] MAC = AA:BB:CC:DD:EE:01
[ALARM] MAC = AA:BB:CC:DD:EE:02
[GW] MAC = AA:BB:CC:DD:EE:03
```

Put those into `MAC_GATEWAY`, `MAC_DOOR`, and `MAC_ALARM`. All three boards need the complete file because each side whitelists the other.

> Opening the serial port resets the ESP32-C3. Boot logs may be missed if the monitor opens after reset; use the MAC printed by `esptool read_mac` if needed.

### 3. Build and upload

```bash
pio run -d deviceHub        -t upload --upload-port COM3
pio run -d deviceSensorDoor -t upload --upload-port COM4
pio run -d deviceAlert      -t upload --upload-port COM5
```

`platformio.ini` does not pin a port because Windows reassigns COM numbers. Run `pio device list` and use the port whose hardware ID is the ESP32-C3 (`303A:1001` for native USB, `1A86:7523` for CH340, `10C4:EA60` for CP210x).

> On this PC `COM6`/`COM7` are an **Intel MBIM modem**, not ESP32 boards. Do not flash them.

### 4. MQTT topics

Prefix: `kel1/wsn`.

| Topic | Direction | Payload |
|---|---|---|
| `kel1/wsn/door/state` | gateway → broker | `{"state":"OPEN","seq":12,"ts":1763000000}` (retained) |
| `kel1/wsn/alarm/state` | gateway → broker | `{"state":"ON"}` (retained) |
| `kel1/wsn/system/mode` | broker → gateway | `{"mode":"ARMED"}` / `{"mode":"DISARMED"}` |
| `kel1/wsn/alarm/cmd` | broker → gateway | `{"cmd":"OFF"}` / `{"cmd":"ON"}` |
| `kel1/wsn/node/door/status` | gateway → broker | `{"online":true,"rssi":-60}` (retained) |
| `kel1/wsn/node/alarm/status` | gateway → broker | `{"online":true,"rssi":-55}` (retained) |
| `kel1/wsn/node/gateway/status` | gateway → broker | `{"online":true}` (retained) |

Example commands:

```bash
mosquitto_pub --url mqtts://broker.emqx.io:8883 \
  -t kel1/wsn/system/mode -m '{"mode":"ARMED"}'

mosquitto_pub --url mqtts://broker.emqx.io:8883 \
  -t kel1/wsn/alarm/cmd -m '{"cmd":"OFF"}'
```

## Behavior

- Door reads **GPIO 4 and GPIO 3** (`INPUT_PULLUP`, `LOW` = closed), debounced 50 ms. Every change sends `DOOR_EVENT`; a heartbeat is sent every 30 s carrying the current door state.
- Alarm listens only for `ALARM_CMD` from the gateway MAC, drives:
  - buzzer GPIO **5** (also GPIO 0 and 1 for the old alarm-board wiring)
  - relay GPIO **6**
  - status LED GPIO **8**
  It replies `ACK` and sends a heartbeat every 30 s.
- Gateway validates sender MAC and node id, applies a monotonic `seq` check (a lower `ts` means the node rebooted and resets the guard), publishes door state, and forwards `ALARM_CMD` when ARMED.
- `DISARMED` turns the alarm off. `ARMED` while the door is open turns it on immediately.
- Missing heartbeat for 90 s publishes `online:false` on the node status topic.
- The gateway republishes its own `online:true` status every 30 s; door/alarm statuses are refreshed on each heartbeat.
- **TX power is 8.5 dBm**, set centrally in `espnowBegin()`. The old working IoT firmware used the same value; 19.5 dBm made C3 transmissions unstable and caused `TX FAIL`.

### OLED

SSD1306 128×64, I2C SDA GPIO 6 / SCL GPIO 7, address `0x3C`, refreshed every 500 ms:

```
WSN GATEWAY
WIFI : OK
MQTT : ONLINE
DEV  : 2/2
MODE : DISARMED
DOOR : ONLINE
ALARM: ONLINE
ESP-NOW: 2/2 PEER
```

- `MQTT : OFF rc=N` — TLS/connection failure code from PubSubClient.
- `NO WIFI` — the gateway is not associated to the AP.
- `NO DATA` — node has never been seen since boot.
- `OFFLINE` — last heartbeat older than 90 s.
- `ESP-NOW: x/2 PEER` — how many node peers are registered; `1/2` usually means one MAC is still zero in `common/secrets.h`.

## Troubleshooting

| Symptom | Check |
|---|---|
| Door `TX FAIL`, `DEV : 0/2` | TX power must be **8.5 dBm**; channel must match the AP; MAC/LMK must match on both sides. |
| `ESP-NOW: 1/2 PEER` | One of `MAC_DOOR` / `MAC_ALARM` is all zeros or failed to register. |
| `DOOR : NO DATA` | Hub has never received a heartbeat. Power the node, verify channel 6, then check its serial log. |
| `DOOR : OFFLINE` | Node was online but no heartbeat for 90 s. |
| `MQTT : OFF rc=-2` | Wi-Fi connected but TLS/MQTT failed; check CA, host, port 8883. |
| Upload “port busy” | Close serial monitors first; `COM6`/`COM7` are the Intel modem on this machine. |
| Buzzer silent although `alarm/state` is `ON` | Verify the buzzer GPIO (5/0/1) and whether the module is active-low; change `BUZZER_ACTIVE_LEVEL` in `common/config.h`. |

## Dashboard

```bash
bun install
bun run dev
```

`.env` / `.env.production`:

```
APP_MQTT_HOST=wss://broker.emqx.io:8084/mqtt
APP_MQTT_USER=
APP_MQTT_PASS=
APP_MQTT_TOPIC_PREFIXES=1003250005,kel1/wsn
```

The dashboard subscribes to both the current `kel1/wsn` contract and the legacy `1003250005/report|event` firmware contract. It shows two tables: devices (online/offline, last value) and events (latest 100, newest first).

## Security notes

> `broker.emqx.io` is a public, unauthenticated broker shared by everyone. TLS protects the connection in transit, but any subscriber can read `kel1/wsn/...`. Use a unique topic prefix if interference or privacy matters for your demo.

- ESP-NOW peers are added with `peer.encrypt = true` and per-peer LMK; the PMK is set before peer registration. Unknown MACs are ignored.
- Only the gateway connects to Wi-Fi/MQTT. Door and alarm nodes never connect to the broker.
- MQTT uses TLS with a pinned CA. The firmware has no port-1883 path.
- Keys and credentials live only in `common/secrets.h` (gitignored); `common/secrets.example.h` holds placeholders.

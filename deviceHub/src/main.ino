#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <time.h>

#include "config.h"
#include "secrets.h"
#include "espnow_common.h"
#include "packet.h"

#if MQTT_PORT == 1883
#error "Port 1883 tanpa TLS tidak diizinkan. Gunakan MQTT_PORT 8883 (TLS)."
#endif

struct NodeState {
  const char *id;
  const char *statusTopic;
  const uint8_t *mac;
  bool online;
  uint32_t lastSeen;
  int rssi;
};

NodeState nodes[] = {
  {"door", TOPIC_NODE_DOOR_STATUS, MAC_DOOR, false, 0, 0},
  {"alarm", TOPIC_NODE_ALARM_STATUS, MAC_ALARM, false, 0, 0},
};

constexpr size_t NODE_COUNT = sizeof(nodes) / sizeof(nodes[0]);
constexpr size_t NODE_DOOR_INDEX = 0;
constexpr size_t NODE_ALARM_INDEX = 1;

// Callback ESP-NOW berjalan di task Wi-Fi, bukan loop Arduino.
// Paket masuk antre dulu supaya publish MQTT tetap di loop utama (thread-safe).
struct ReceivedPacket {
  uint8_t mac[6];
  Packet packet;
  int rssi;
};

constexpr size_t RX_QUEUE_SIZE = 8;
ReceivedPacket rxQueue[RX_QUEUE_SIZE];
volatile uint8_t rxHead = 0;
volatile uint8_t rxTail = 0;
portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t rxDropped = 0;

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

bool displayReady = false;
uint32_t lastDisplayAt = 0;

bool armed = false;
bool doorOpen = false;
bool alarmKnown = false;
uint8_t alarmState = 0;
uint32_t commandSequence = 0;
ReplayGuard doorGuard;
ReplayGuard alarmGuard;

uint32_t lastWifiAttempt = 0;
uint32_t lastMqttAttempt = 0;
uint32_t lastEspNowAttempt = 0;
uint32_t lastGatewayStatus = 0;
bool espnowStarted = false;
bool doorPeerReady = false;
bool alarmPeerReady = false;

void enqueuePacket(const uint8_t *mac, const Packet &packet, int rssi) {
  portENTER_CRITICAL(&rxMux);

  const uint8_t next = (rxHead + 1) % RX_QUEUE_SIZE;
  if (next == rxTail) {
    rxDropped++;
  } else {
    memcpy(rxQueue[rxHead].mac, mac, 6);
    rxQueue[rxHead].packet = packet;
    rxQueue[rxHead].rssi = rssi;
    rxHead = next;
  }

  portEXIT_CRITICAL(&rxMux);
}

bool dequeuePacket(ReceivedPacket &out) {
  bool available = false;

  portENTER_CRITICAL(&rxMux);
  if (rxTail != rxHead) {
    out = rxQueue[rxTail];
    rxTail = (rxTail + 1) % RX_QUEUE_SIZE;
    available = true;
  }
  portEXIT_CRITICAL(&rxMux);

  return available;
}

void onMqttMessage(char *topic, byte *payload, unsigned int length);

bool publishJson(const char *topic, JsonDocument &doc, bool retained = false) {
  if (!mqtt.connected()) {
    return false;
  }

  char payload[192];
  const size_t length = serializeJson(doc, payload, sizeof(payload));
  if (length == 0) {
    Serial.printf("[GW] serialisasi JSON gagal untuk %s\r\n", topic);
    return false;
  }

  const bool ok = mqtt.publish(topic, payload, retained);
  if (!ok) {
    Serial.printf("[GW] publish gagal: %s\r\n", topic);
  }

  return ok;
}

void publishNodeStatus(size_t index, bool online) {
  JsonDocument doc;
  doc["online"] = online;

  if (online && nodes[index].rssi != 0) {
    doc["rssi"] = nodes[index].rssi;
  }

  publishJson(nodes[index].statusTopic, doc, true);
}

void publishGatewayStatus() {
  JsonDocument doc;
  doc["online"] = true;

  publishJson(TOPIC_NODE_GATEWAY_STATUS, doc, true);
  lastGatewayStatus = millis();
}

void refreshGatewayStatus() {
  if (!mqtt.connected()) {
    return;
  }
  if (lastGatewayStatus != 0 && millis() - lastGatewayStatus < HEARTBEAT_MS) {
    return;
  }

  publishGatewayStatus();
}

void publishDoorState(uint32_t seq, bool force) {
  static bool published = false;
  static bool lastOpen = false;

  if (!force && published && doorOpen == lastOpen) {
    return;
  }

  JsonDocument doc;
  doc["state"] = doorOpen ? "OPEN" : "CLOSED";
  doc["seq"] = seq;
  doc["ts"] = static_cast<uint32_t>(time(nullptr));

  if (publishJson(TOPIC_DOOR_STATE, doc, true)) {
    published = true;
    lastOpen = doorOpen;
  }
}

void publishAlarmState() {
  JsonDocument doc;
  doc["state"] = alarmState ? "ON" : "OFF";

  publishJson(TOPIC_ALARM_STATE, doc, true);
}

void sendAck(const uint8_t *mac, uint32_t seq, uint8_t value) {
  if (!doorPeerReady) {
    return;
  }

  Packet ack = makePacket(PACKET_ACK, NODE_GATEWAY, seq, value);
  esp_now_send(mac, reinterpret_cast<uint8_t *>(&ack), sizeof(ack));
}

void sendAlarmCommand(uint8_t value) {
  if (!alarmPeerReady) {
    Serial.println("[GW] ALARM_CMD ditunda: MAC_ALARM/peer belum siap");
    return;
  }

  Packet packet = makePacket(PACKET_ALARM_CMD, NODE_GATEWAY, ++commandSequence, value);
  const esp_err_t result = esp_now_send(MAC_ALARM, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));

  Serial.printf("[GW] ALARM_CMD %s seq=%lu -> %s\r\n",
                value ? "ON" : "OFF",
                static_cast<unsigned long>(packet.seq),
                result == ESP_OK ? "OK" : "GAGAL");
}

void markNodeSeen(size_t index, int rssi) {
  NodeState &node = nodes[index];
  const bool wasOnline = node.online;

  node.lastSeen = millis();
  node.online = true;
  if (rssi != 0) {
    node.rssi = rssi;
  }

  if (!wasOnline) {
    Serial.printf("[GW] node %s online\r\n", node.id);
    publishNodeStatus(index, true);
  }
}

void handleDoorPacket(const Packet &packet, int rssi) {
  if (!doorGuard.accept(packet)) {
    Serial.printf("[GW] door seq lama %lu ditolak\r\n", static_cast<unsigned long>(packet.seq));
    return;
  }

  markNodeSeen(NODE_DOOR_INDEX, rssi);

  if (packet.type == PACKET_DOOR_EVENT || packet.type == PACKET_HEARTBEAT) {
    const bool open = packet.value != 0;
    const bool changed = open != doorOpen;

    doorOpen = open;

    // Kirim perintah alarm dulu supaya buzzer berbunyi sebelum publish TLS.
    if (armed && doorOpen && alarmState == 0) {
      sendAlarmCommand(1);
    }

    if (changed || packet.type == PACKET_DOOR_EVENT) {
      publishDoorState(packet.seq, true);
    }

    if (packet.type == PACKET_HEARTBEAT) {
      publishNodeStatus(NODE_DOOR_INDEX, true);
    }
  }

  sendAck(MAC_DOOR, packet.seq, doorOpen ? 1 : 0);
}

void handleAlarmPacket(const Packet &packet, int rssi) {
  if (!alarmGuard.accept(packet)) {
    Serial.printf("[GW] alarm seq lama %lu ditolak\r\n", static_cast<unsigned long>(packet.seq));
    return;
  }

  markNodeSeen(NODE_ALARM_INDEX, rssi);

  if (packet.type == PACKET_ACK || packet.type == PACKET_HEARTBEAT) {
    const uint8_t state = packet.value != 0 ? 1 : 0;

    if (!alarmKnown || state != alarmState) {
      alarmState = state;
      alarmKnown = true;
      publishAlarmState();
    }
  }

  if (packet.type == PACKET_HEARTBEAT) {
    publishNodeStatus(NODE_ALARM_INDEX, true);
  }
}

void handlePacket(const uint8_t *mac, const Packet &packet, int rssi) {
  if (macEquals(mac, MAC_DOOR) && packet.nodeId == NODE_DOOR) {
    Serial.printf("[GW] rx door type=%u seq=%lu value=%u\r\n",
                  packet.type,
                  static_cast<unsigned long>(packet.seq),
                  packet.value);
    handleDoorPacket(packet, rssi);
    return;
  }

  if (macEquals(mac, MAC_ALARM) && packet.nodeId == NODE_ALARM) {
    Serial.printf("[GW] rx alarm type=%u seq=%lu value=%u\r\n",
                  packet.type,
                  static_cast<unsigned long>(packet.seq),
                  packet.value);
    handleAlarmPacket(packet, rssi);
    return;
  }

  Serial.printf("[GW] paket dari MAC tidak dikenal: %02X:%02X:%02X:%02X:%02X:%02X node=%u\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], packet.nodeId);
}

void processEspNowQueue() {
  ReceivedPacket received;

  while (dequeuePacket(received)) {
    handlePacket(received.mac, received.packet, received.rssi);
  }

  uint32_t dropped = 0;
  portENTER_CRITICAL(&rxMux);
  dropped = rxDropped;
  rxDropped = 0;
  portEXIT_CRITICAL(&rxMux);

  if (dropped > 0) {
    Serial.printf("[GW] %lu paket ESP-NOW terbuang (antrean penuh)\r\n",
                  static_cast<unsigned long>(dropped));
  }
}

void handleWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  if (millis() - lastWifiAttempt < WIFI_RETRY_MS) {
    return;
  }

  lastWifiAttempt = millis();
  Serial.printf("[GW] Wi-Fi connect ke '%s'...\r\n", WIFI_SSID);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void ensureEspNow() {
  if (doorPeerReady && alarmPeerReady) {
    return;
  }
  if (millis() - lastEspNowAttempt < WIFI_RETRY_MS) {
    return;
  }

  lastEspNowAttempt = millis();

  if (!espnowStarted) {
    if (!espnowBegin(0, ESPNOW_PMK, false)) {
      Serial.println("[GW] ESP-NOW init gagal, dicoba lagi");
      return;
    }

    espnowStarted = true;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    const esp_err_t recvResult = esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
      if (len != static_cast<int>(sizeof(Packet))) {
        return;
      }

      Packet packet;
      memcpy(&packet, data, sizeof(packet));
      enqueuePacket(info->src_addr, packet, info->rx_ctrl->rssi);
    });
#else
    const esp_err_t recvResult = esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
      if (len != static_cast<int>(sizeof(Packet))) {
        return;
      }

      Packet packet;
      memcpy(&packet, data, sizeof(packet));
      enqueuePacket(mac, packet, 0);
    });
#endif

    Serial.printf("[GW] recv cb=%d\r\n", recvResult);
    Serial.printf("[GW] ESP-NOW channel %u (config %u)\r\n", WiFi.channel(), ESPNOW_CHANNEL);
  }

  if (!doorPeerReady && !macIsEmpty(MAC_DOOR)) {
    doorPeerReady = espnowAddPeer(MAC_DOOR, ESPNOW_LMK_DOOR, WiFi.channel());
    if (doorPeerReady) {
      Serial.println("[GW] peer door OK");
    }
  }
  if (!alarmPeerReady && !macIsEmpty(MAC_ALARM)) {
    alarmPeerReady = espnowAddPeer(MAC_ALARM, ESPNOW_LMK_ALARM, WiFi.channel());
    if (alarmPeerReady) {
      Serial.println("[GW] peer alarm OK");
    }
  }

  if (doorPeerReady && alarmPeerReady) {
    Serial.println("[GW] ESP-NOW siap (2 peer terenkripsi)");
  }
}

void ensureMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) {
    return;
  }
  if (millis() - lastMqttAttempt < MQTT_RETRY_MS) {
    return;
  }

  lastMqttAttempt = millis();
  const String clientId = String("wsn-gateway-") + WiFi.macAddress();

  Serial.printf("[GW] MQTT TLS %s:%u ...\r\n", MQTT_HOST, MQTT_PORT);

  const bool connected = strlen(MQTT_USER) > 0
    ? mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)
    : mqtt.connect(clientId.c_str());

  if (!connected) {
    Serial.printf("[GW] MQTT gagal rc=%d\r\n", mqtt.state());
    return;
  }

  Serial.println("[GW] MQTT terhubung (TLS)");
  mqtt.subscribe(TOPIC_SYSTEM_MODE);
  mqtt.subscribe(TOPIC_ALARM_CMD);

  publishNodeStatus(NODE_DOOR_INDEX, nodes[NODE_DOOR_INDEX].online);
  publishNodeStatus(NODE_ALARM_INDEX, nodes[NODE_ALARM_INDEX].online);
  publishGatewayStatus();

  if (alarmKnown) {
    publishAlarmState();
  }
}

void checkNodesOffline() {
  const uint32_t now = millis();

  for (size_t i = 0; i < NODE_COUNT; i++) {
    NodeState &node = nodes[i];

    if (!node.online) {
      continue;
    }
    if (now - node.lastSeen <= NODE_OFFLINE_MS) {
      continue;
    }

    node.online = false;
    Serial.printf("[GW] node %s offline (heartbeat >90 detik)\r\n", node.id);
    publishNodeStatus(i, false);
  }
}

size_t onlineNodeCount() {
  size_t count = 0;

  for (size_t i = 0; i < NODE_COUNT; i++) {
    if (nodes[i].online) {
      count++;
    }
  }

  return count;
}

const char *nodeLabel(size_t index) {
  if (nodes[index].online) {
    return "ONLINE";
  }
  if (nodes[index].lastSeen == 0) {
    return "NO DATA";
  }

  return "OFFLINE";
}

void refreshDisplay() {
  if (!displayReady) {
    return;
  }
  if (lastDisplayAt != 0 && millis() - lastDisplayAt < DISPLAY_REFRESH_MS) {
    return;
  }

  lastDisplayAt = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println(F("WSN GATEWAY"));
  display.printf("WIFI : %s\n", WiFi.status() == WL_CONNECTED ? "OK" : "DOWN");

  if (WiFi.status() != WL_CONNECTED) {
    display.println(F("MQTT : NO WIFI"));
  } else if (mqtt.connected()) {
    display.println(F("MQTT : ONLINE"));
  } else {
    display.printf("MQTT : OFF rc=%d\n", mqtt.state());
  }

  display.printf("DEV  : %u/%u\n",
                 static_cast<unsigned>(onlineNodeCount()),
                 static_cast<unsigned>(NODE_COUNT));
  display.printf("MODE : %s\n", armed ? "ARMED" : "DISARMED");
  display.printf("DOOR : %s\n", nodeLabel(NODE_DOOR_INDEX));
  display.printf("ALARM: %s\n", nodeLabel(NODE_ALARM_INDEX));
  display.printf("ESP-NOW: %u/%u PEER\n",
                 static_cast<unsigned>((doorPeerReady ? 1 : 0) + (alarmPeerReady ? 1 : 0)),
                 static_cast<unsigned>(NODE_COUNT));

  display.display();
}

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.printf("[GW] JSON MQTT salah: %s\r\n", error.c_str());
    return;
  }

  if (strcmp(topic, TOPIC_SYSTEM_MODE) == 0) {
    const String mode = doc["mode"] | "";

    if (mode.equalsIgnoreCase("ARMED")) {
      armed = true;
      Serial.println("[GW] mode = ARMED");

      if (doorOpen && alarmState == 0) {
        sendAlarmCommand(1);
      }
    } else if (mode.equalsIgnoreCase("DISARMED")) {
      armed = false;
      Serial.println("[GW] mode = DISARMED");
      sendAlarmCommand(0);
    }

    return;
  }

  if (strcmp(topic, TOPIC_ALARM_CMD) == 0) {
    const String command = doc["cmd"] | "";

    if (command.equalsIgnoreCase("OFF")) {
      sendAlarmCommand(0);
    } else if (command.equalsIgnoreCase("ON")) {
      sendAlarmCommand(1);
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println("\r\n[BOOT] deviceHub gateway (ESP-NOW <-> MQTT/TLS)");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[GW] MAC = %s\r\n", WiFi.macAddress().c_str());

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  if (displayReady) {
    display.setTextWrap(false);
    display.clearDisplay();
    display.display();
    Serial.println("[GW] OLED siap");
  } else {
    Serial.println("[GW] OLED tidak terdeteksi, lanjut tanpa display");
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  secureClient.setCACert(MQTT_CA_CERT);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);

  lastWifiAttempt = millis();
}

void loop() {
  handleWifi();

  if (WiFi.status() == WL_CONNECTED) {
    ensureEspNow();
    processEspNowQueue();
    ensureMqtt();

    if (mqtt.connected()) {
      mqtt.loop();
      refreshGatewayStatus();
    }

    checkNodesOffline();
  }

  refreshDisplay();
  delay(10);
}

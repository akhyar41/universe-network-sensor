#include <Arduino.h>
#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_now.h>

#include "config.h"
#include "secrets.h"
#include "espnow_common.h"
#include "packet.h"

uint32_t sequence = 0;
uint32_t lastHeartbeat = 0;
volatile uint8_t txResult = 0;
bool linkUp = false;

int lastReading = HIGH;
int stableReading = HIGH;
uint32_t lastEdgeMs = 0;

bool isDoorOpen(uint8_t reading) {
  const bool physicallyClosed = reading == REED_CLOSED_LEVEL;
  return DOOR_INVERT ? physicallyClosed : !physicallyClosed;
}

// Dua pin didukung: reed utama GPIO4 dan switch simulasi GPIO3.
// Pin yang tidak dipasang tetap HIGH karena INPUT_PULLUP.
int readDoorLevel() {
  if (digitalRead(PIN_REED_SWITCH) == REED_CLOSED_LEVEL) {
    return REED_CLOSED_LEVEL;
  }
  if (digitalRead(PIN_REED_SWITCH_ALT) == REED_CLOSED_LEVEL) {
    return REED_CLOSED_LEVEL;
  }

  return HIGH;
}

void sendPacket(uint8_t type, uint8_t value) {
  Packet packet = makePacket(type, NODE_DOOR, ++sequence, value);

  const esp_err_t result = esp_now_send(MAC_GATEWAY, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  if (result != ESP_OK) {
    linkUp = false;
  }
  Serial.printf("[DOOR] kirim type=%u value=%u seq=%lu -> %s\r\n",
                type,
                value,
                static_cast<unsigned long>(packet.seq),
                result == ESP_OK ? "OK" : "GAGAL");
}

void handlePacket(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != static_cast<int>(sizeof(Packet))) {
    return;
  }
  if (!macEquals(mac, MAC_GATEWAY)) {
    return;
  }

  Packet packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.nodeId != NODE_GATEWAY || packet.type != PACKET_ACK) {
    return;
  }

  Serial.printf("[DOOR] ACK seq=%lu\r\n", static_cast<unsigned long>(packet.seq));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println("\r\n[BOOT] deviceSensorDoor (ESP-NOW)");

  pinMode(PIN_REED_SWITCH, INPUT_PULLUP);
  pinMode(PIN_REED_SWITCH_ALT, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("[DOOR] MAC = %s\r\n", WiFi.macAddress().c_str());

  if (!espnowBegin(ESPNOW_CHANNEL, ESPNOW_PMK, true)) {
    Serial.println("[DOOR] ESP-NOW gagal, restart...");
    delay(3000);
    ESP.restart();
  }
  if (!espnowAddPeer(MAC_GATEWAY, ESPNOW_LMK_DOOR, ESPNOW_CHANNEL)) {
    Serial.println("[DOOR] peer gateway gagal, cek MAC_GATEWAY/LMK");
  } else {
    Serial.println("[DOOR] peer gateway OK");
  }

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    handlePacket(info->src_addr, data, len);
  });
  esp_now_register_send_cb([](const wifi_tx_info_t *, esp_now_send_status_t status) {
    txResult = status == ESP_NOW_SEND_SUCCESS ? 1 : 2;
  });
#else
  esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
    handlePacket(mac, data, len);
  });
  esp_now_register_send_cb([](const uint8_t *, esp_now_send_status_t status) {
    txResult = status == ESP_NOW_SEND_SUCCESS ? 1 : 2;
  });
#endif

  stableReading = readDoorLevel();
  lastReading = stableReading;
  lastEdgeMs = millis();

  Serial.printf("[DOOR] kondisi awal: %s\r\n", isDoorOpen(stableReading) ? "TERBUKA" : "TERTUTUP");
  sendPacket(PACKET_DOOR_EVENT, isDoorOpen(stableReading) ? 1 : 0);
  lastHeartbeat = millis();
}

void pollLink() {
  const uint8_t result = txResult;
  if (result == 0) {
    return;
  }
  txResult = 0;
  linkUp = result == 1;
}

void loop() {
  pollLink();

  const int reading = readDoorLevel();
  if (reading != lastReading) {
    lastReading = reading;
    lastEdgeMs = millis();
  }

  if (millis() - lastEdgeMs >= DEBOUNCE_MS && reading != stableReading) {
    stableReading = reading;

    const uint8_t value = isDoorOpen(stableReading) ? 1 : 0;
    Serial.printf("[DOOR] pintu %s\r\n", value ? "TERBUKA" : "TERTUTUP");
    sendPacket(PACKET_DOOR_EVENT, value);
    lastHeartbeat = millis();
  }

  if (millis() - lastHeartbeat >= (linkUp ? HEARTBEAT_MS : LINK_RETRY_MS)) {
    lastHeartbeat = millis();
    sendPacket(PACKET_HEARTBEAT, isDoorOpen(stableReading) ? 1 : 0);
  }
}

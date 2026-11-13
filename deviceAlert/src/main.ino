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
volatile bool alarmOn = false;
volatile bool ackPending = false;
ReplayGuard commandGuard;

void applyAlarm(bool on) {
  alarmOn = on;

  digitalWrite(PIN_RELAY, on ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
  digitalWrite(PIN_BUZZER, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
  digitalWrite(PIN_BUZZER_ALT_A, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
  digitalWrite(PIN_BUZZER_ALT_B, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
  digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW);

  Serial.printf("[ALARM] %s\r\n", on ? "ON" : "OFF");
}

void sendPacket(uint8_t type, uint8_t value) {
  Packet packet = makePacket(type, NODE_ALARM, ++sequence, value);

  const esp_err_t result = esp_now_send(MAC_GATEWAY, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  Serial.printf("[ALARM] kirim type=%u value=%u seq=%lu -> %s\r\n",
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
    Serial.println("[ALARM] paket dari MAC tidak dikenal, diabaikan");
    return;
  }

  Packet packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.nodeId != NODE_GATEWAY) {
    return;
  }
  if (packet.type != PACKET_ALARM_CMD) {
    return;
  }
  if (!commandGuard.accept(packet)) {
    Serial.printf("[ALARM] seq lama %lu ditolak\r\n", static_cast<unsigned long>(packet.seq));
    return;
  }

  applyAlarm(packet.value != 0);
  ackPending = true;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println("\r\n[BOOT] deviceAlert (ESP-NOW)");

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUZZER_ALT_A, OUTPUT);
  pinMode(PIN_BUZZER_ALT_B, OUTPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);
  applyAlarm(false);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("[ALARM] MAC = %s\r\n", WiFi.macAddress().c_str());

  if (!espnowBegin(ESPNOW_CHANNEL, ESPNOW_PMK, true)) {
    Serial.println("[ALARM] ESP-NOW gagal, restart...");
    delay(3000);
    ESP.restart();
  }
  if (!espnowAddPeer(MAC_GATEWAY, ESPNOW_LMK_ALARM, ESPNOW_CHANNEL)) {
    Serial.println("[ALARM] peer gateway gagal, cek MAC_GATEWAY/LMK");
  }

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_now_register_recv_cb([](const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    handlePacket(info->src_addr, data, len);
  });
#else
  esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
    handlePacket(mac, data, len);
  });
#endif

  sendPacket(PACKET_HEARTBEAT, alarmOn ? 1 : 0);
  lastHeartbeat = millis();
}

void loop() {
  if (ackPending) {
    ackPending = false;
    sendPacket(PACKET_ACK, alarmOn ? 1 : 0);
  }

  if (millis() - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = millis();
    sendPacket(PACKET_HEARTBEAT, alarmOn ? 1 : 0);
  }
}

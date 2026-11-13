#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "config.h"
#include "packet.h"

// Mulai ESP-NOW.
// setChannel = true untuk node sensor (tidak terhubung AP).
// setChannel = false untuk gateway (channel mengikuti AP).
inline bool espnowBegin(uint8_t channel, const uint8_t *pmk, bool setChannel) {
  // Tuning sama seperti firmware lama: 8.5 dBm lebih stabil di board C3.
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  Serial.printf("[ESP-NOW] TX power = %.2f dBm\r\n", WiFi.getTxPower() / 4.0);

  if (setChannel) {
    // Trik IDF: set channel saat promiscuous agar channel TX/RX benar-benar ikut.
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  }

  const esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    Serial.printf("[ESP-NOW] init gagal (%d)\r\n", initResult);
    return false;
  }

  // Set lagi setelah init: sebagian IDF memakai "home channel" untuk TX.
  if (setChannel) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  }

  const esp_err_t pmkResult = esp_now_set_pmk(pmk);
  if (pmkResult != ESP_OK) {
    Serial.printf("[ESP-NOW] set PMK gagal (%d)\r\n", pmkResult);
    return false;
  }

  // ESP-NOW butuh radio aktif terus dan bandwidth 20 MHz.
  // HT40 pada AP sering membuat frame ESP-NOW tidak diterima.
  const esp_err_t psResult = esp_wifi_set_ps(WIFI_PS_NONE);
  const esp_err_t bwResult = esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
  const esp_err_t protoResult = esp_wifi_set_protocol(
    WIFI_IF_STA,
    WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N
  );

  Serial.printf("[ESP-NOW] ps=%d bw=%d proto=%d\r\n", psResult, bwResult, protoResult);

  return true;
}

inline bool macEquals(const uint8_t *left, const uint8_t *right) {
  return memcmp(left, right, 6) == 0;
}

inline bool macIsEmpty(const uint8_t *mac) {
  for (uint8_t i = 0; i < 6; i++) {
    if (mac[i] != 0) {
      return false;
    }
  }

  return true;
}

// Tambah peer terenkripsi AES-128 (LMK).
// channel = 0 mengikuti channel Wi-Fi aktif; node yang tidak terhubung AP
// sebaiknya mengisi channel ESP-NOW secara eksplisit.
inline bool espnowAddPeer(const uint8_t *mac, const uint8_t *lmk, uint8_t channel = 0) {
  if (macIsEmpty(mac)) {
    Serial.println("[ESP-NOW] MAC peer masih kosong, isi common/secrets.h");
    return false;
  }
  if (esp_now_is_peer_exist(mac)) {
    return true;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = channel;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = ESPNOW_ENCRYPT != 0;
  memcpy(peer.lmk, lmk, 16);

  const esp_err_t result = esp_now_add_peer(&peer);
  if (result != ESP_OK) {
    Serial.printf("[ESP-NOW] add peer gagal (%d)\r\n", result);
    return false;
  }

  return true;
}

inline Packet makePacket(uint8_t type, uint8_t nodeId, uint32_t seq, uint8_t value) {
  Packet packet;
  packet.type = type;
  packet.nodeId = nodeId;
  packet.seq = seq;
  packet.ts = millis();
  packet.value = value;
  return packet;
}

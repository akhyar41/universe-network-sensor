#pragma once

#include <stdint.h>

// ============================================================
// TEMPLATE. Salin ke common/secrets.h lalu isi nilai aslinya.
// common/secrets.h sudah masuk .gitignore, jangan pernah di-commit.
// ============================================================

// Wi-Fi AP yang channel-nya dipakai ESP-NOW
#define WIFI_SSID "SmartHub"
#define WIFI_PASSWORD ""

// Broker MQTT TLS publik untuk mahasiswa.
// ESP32 memakai TCP TLS 8883 (PubSubClient). Port 8084 hanya untuk WebSocket
// browser/dashboard (wss://broker.emqx.io:8084/mqtt), bukan untuk firmware.
#define MQTT_HOST "broker.emqx.io"
#define MQTT_PORT 8883
#define MQTT_USER ""
#define MQTT_PASSWORD ""

// CA certificate broker: DigiCert Global Root G2 (dipakai broker.emqx.io).
static const char MQTT_CA_CERT[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// Channel 2.4 GHz router/AP. Harus sama dengan channel AP gateway.
#define ESPNOW_CHANNEL 6

// Kunci AES-128 (16 byte). Ganti semua nilai ini, jangan pakai contoh.
static const uint8_t ESPNOW_PMK[16] = {
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
  0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
};

static const uint8_t ESPNOW_LMK_DOOR[16] = {
  0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
  0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01,
};

static const uint8_t ESPNOW_LMK_ALARM[16] = {
  0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, 0x78,
  0x87, 0x96, 0xA5, 0xB4, 0xC3, 0xD2, 0xE1, 0xF0,
};

// MAC ESP32-C3 masing-masing. Baca dari Serial saat boot: "[DOOR] MAC = ...".
static const uint8_t MAC_GATEWAY[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t MAC_DOOR[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t MAC_ALARM[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

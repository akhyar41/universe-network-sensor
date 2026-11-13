#pragma once

#include <Arduino.h>

// Pin default (AGENTS.md bagian 2)
constexpr uint8_t PIN_REED_SWITCH = 4;
constexpr uint8_t PIN_REED_SWITCH_ALT = 3; // board switch simulasi (opsional)
constexpr uint8_t PIN_BUZZER = 5;
constexpr uint8_t PIN_BUZZER_ALT_A = 0; // board alarm lama / wiring 0/1
constexpr uint8_t PIN_BUZZER_ALT_B = 1;
constexpr uint8_t PIN_RELAY = 6;
constexpr uint8_t PIN_STATUS_LED = 8;

// OLED SSD1306 gateway (I2C)
constexpr uint8_t PIN_OLED_SDA = 6;
constexpr uint8_t PIN_OLED_SCL = 7;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint32_t DISPLAY_REFRESH_MS = 500;

// Reed switch INPUT_PULLUP: LOW saat switch aktif
constexpr uint8_t REED_CLOSED_LEVEL = LOW;
// Balik logika pintu jika modul switch memberi polarity terbalik.
constexpr bool DOOR_INVERT = true;

// Modul buzzer/relay aktif HIGH
constexpr uint8_t BUZZER_ACTIVE_LEVEL = HIGH;
constexpr uint8_t RELAY_ACTIVE_LEVEL = HIGH;

// Enkripsi ESP-NOW (PMK+LMK). Default aktif; override lewat secrets.h.
#ifndef ESPNOW_ENCRYPT
#define ESPNOW_ENCRYPT 1
#endif

// Durasi alarm menyala setelah trigger sensor (3 menit).
constexpr uint32_t ALARM_ON_DURATION_MS = 3UL * 60UL * 1000UL;

// Waktu (ms)
constexpr uint32_t DEBOUNCE_MS = 50;
constexpr uint32_t HEARTBEAT_MS = 30000;
constexpr uint32_t NODE_OFFLINE_MS = 90000;
constexpr uint32_t WIFI_RETRY_MS = 5000;
constexpr uint32_t MQTT_RETRY_MS = 5000;
constexpr unsigned long SERIAL_BAUD = 115200;

// Topik MQTT (prefix AGENTS.md)
#define MQTT_PREFIX "kel1/wsn"

constexpr char TOPIC_DOOR_STATE[] = MQTT_PREFIX "/door/state";
constexpr char TOPIC_ALARM_STATE[] = MQTT_PREFIX "/alarm/state";
constexpr char TOPIC_SYSTEM_MODE[] = MQTT_PREFIX "/system/mode";
constexpr char TOPIC_ALARM_CMD[] = MQTT_PREFIX "/alarm/cmd";
constexpr char TOPIC_NODE_DOOR_STATUS[] = MQTT_PREFIX "/node/door/status";
constexpr char TOPIC_NODE_ALARM_STATUS[] = MQTT_PREFIX "/node/alarm/status";
constexpr char TOPIC_NODE_GATEWAY_STATUS[] = MQTT_PREFIX "/node/gateway/status";

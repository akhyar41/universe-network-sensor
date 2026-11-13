#pragma once

#include <Arduino.h>

// Tipe paket ESP-NOW (AGENTS.md bagian 6)
enum PacketType : uint8_t {
  PACKET_DOOR_EVENT = 1,
  PACKET_ALARM_CMD = 2,
  PACKET_HEARTBEAT = 3,
  PACKET_ACK = 4,
};

// ID node
enum NodeId : uint8_t {
  NODE_GATEWAY = 0,
  NODE_DOOR = 1,
  NODE_ALARM = 2,
};

// Payload biner ESP-NOW (maks 250 byte)
typedef struct __attribute__((packed)) {
  uint8_t type;     // PacketType
  uint8_t nodeId;   // NodeId pengirim
  uint32_t seq;     // nomor urut, naik terus
  uint32_t ts;      // millis() pengirim
  uint8_t value;    // 0=CLOSED/OFF, 1=OPEN/ON; ACK membawa state saat ini
} Packet;

static_assert(sizeof(Packet) == 11, "Packet harus tepat 11 byte");

// Anti-replay sederhana: seq harus naik. ts yang turun menandakan node reboot.
struct ReplayGuard {
  bool hasLast = false;
  uint32_t lastSeq = 0;
  uint32_t lastTs = 0;

  bool accept(const Packet &packet) {
    if (!hasLast) {
      hasLast = true;
      lastSeq = packet.seq;
      lastTs = packet.ts;
      return true;
    }

    const bool rebooted = packet.ts + 1000UL < lastTs;
    if (!rebooted && packet.seq <= lastSeq) {
      return false;
    }

    lastSeq = packet.seq;
    lastTs = packet.ts;
    return true;
  }
};

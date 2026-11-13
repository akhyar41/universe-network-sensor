# Ringkasan Investigasi & Implementasi Proyek WSN

Tanggal: 13 November 2026 (Asia/Jakarta)
Kondisi akhir perangkat: hub + pintu + alarm online, MQTT TLS berjalan, pengujian ARM/DISARM berhasil.

## 1. Status akhir

| Node | Port USB | MAC | Kondisi |
|---|---|---|---|
| Hub / gateway | `COM3` | `F0:F5:BD:FD:B5:D4` | Wi-Fi + MQTT TLS online, OLED aktif |
| Pintu | `COM4` | `88:56:A6:5C:8B:C0` | ESP-NOW terenkripsi, heartbeat OK |
| Alarm | `COM5` | `88:56:A6:5C:80:20` | ESP-NOW terenkripsi, output buzzer/relay aktif |

Terverifikasi di broker (`kel1/wsn/#`):

```text
kel1/wsn/node/gateway/status {"online":true}
kel1/wsn/node/door/status    {"online":true}
kel1/wsn/node/alarm/status   {"online":true}
kel1/wsn/door/state          {"state":"CLOSED","seq":3,"ts":...}
kel1/wsn/alarm/state         {"state":"OFF"}
```

OLED seharusnya menampilkan `DEV : 2/2`, `DOOR : ONLINE`, `ALARM: ONLINE`, dan `ESP-NOW: 2/2 PEER`.

Pengujian ARM/DISARM dilakukan sebelum polaritas pintu dibalik, saat `door/state` masih `OPEN`:

```text
OFF -> ARMED -> ON -> DISARMED -> OFF
```

Setelah `DOOR_INVERT=true`, kondisi idle menjadi `CLOSED`; untuk menguji alarm, ubah switch sampai `door/state` menjadi `OPEN`, lalu kirim `ARMED`. Aturan sistem tetap: ARMED + pintu terbuka menyalakan alarm; DISARMED mematikan alarm.

## 2. Temuan per kasus

### Kasus 1 — Dashboard
- Dashboard SvelteKit dibuat dengan dua tabel: perangkat dan event.
- MQTT melalui WebSocket menggunakan `mqtt.js`; Dexie menyimpan perangkat/event secara lokal.
- Mengonsumsi topik `kel1/wsn/...` sekaligus kontrak lama `1003250005/report|event`.
- Diuji dengan pesan MQTT simulasi: baris muncul, data bertahan setelah reload, perangkat menjadi offline setelah 90 detik.
- Status: berfungsi.

### Kasus 2 — AGENTS.md
- Diterjemahkan sepenuhnya ke bahasa Inggris.
- Struktur repo, kontrak ESP-NOW/MQTT, bagian dashboard, dan konfigurasi broker publik diperbarui.
- Status: selesai.

### Kasus 3 — Port COM6/COM7 (tidak dipakai)
- `COM6`/`COM7` di PC ini adalah modem Intel MBIM, bukan board ESP32; `COM11`/`COM12` adalah link Bluetooth.
- Tidak dipakai dalam setup akhir. Port yang benar untuk ketiga board adalah `COM3` (hub), `COM4` (pintu), dan `COM5` (alarm).
- Entri `upload_port` tetap dihapus dari `platformio.ini` agar upload tidak salah target.
- Status: tidak relevan untuk sistem final.

### Kasus 4 — `TX FAIL` ESP-NOW pada node pintu
Gejala:

```text
[DOOR] peer gateway OK
[DOOR] ESP-NOW channel 6
[DOOR] kirim type=1 value=1 seq=1 -> OK
[DOOR] TX FAIL
```

Hub tidak menerima apa pun, sehingga `DEV : 0/2` selalu nol.

Pengujian yang dilakukan:
- Memastikan kedua sisi berada di channel 6; AP SmartHub memakai channel 6.
- Memastikan MAC gateway dan MAC pintu tertanam benar di binary kedua firmware.
- Memastikan byte PMK/LMK sama di kedua binary.
- Registrasi callback penerimaan berhasil (`recv cb=0`).
- Mencoba channel peer eksplisit, pengaturan channel lewat promiscuous, bandwidth HT20, dan mematikan power-save.
- Mencoba firmware hub khusus ESP-NOW dengan Wi-Fi dimatikan total.
- Mencoba broadcast dan unicast tanpa enkripsi.
- Mencoba receiver kedua di board alarm.
- Hub → pintu berhasil (pintu menerima ACK hub), pintu → hub tidak pernah berhasil.
- **Akar masalah: daya TX.** Firmware baru memakai `WIFI_POWER_19_5dBm`; firmware IoT lama yang bekerja memakai `WIFI_POWER_8_5dBm`. Pada 19.5 dBm jalur transmit C3 tidak stabil dan semua pengiriman gagal.
- Perbaikan: `WiFi.setTxPower(WIFI_POWER_8_5dBm)` dipusatkan di `espnowBegin()`.
- Setelah ketiga board di-flash ulang, heartbeat pintu dan alarm sampai ke hub.
- Status: selesai.

### Kasus 5 — Tes enkripsi menyesatkan
- `#define ESPNOW_ENCRYPT 0` awalnya diletakkan di `secrets.h`.
- `secrets.h` di-include **setelah** `espnow_common.h`, sehingga saat header dikompilasi nilainya masih default `1`.
- Akibatnya, tes yang dianggap “tanpa enkripsi” sebenarnya tetap terenkripsi.
- Urutan include diperbaiki: `config.h`, `secrets.h`, `espnow_common.h`, `packet.h`.
- Firmware final memakai enkripsi aktif (default `1`).
- Status: selesai.

### Kasus 6 — Status OLED
- Dukungan SSD1306 128×64 ditambahkan pada I2C SDA GPIO 6 / SCL GPIO 7, alamat `0x3C`.
- Menampilkan:
  - Status Wi-Fi
  - Status MQTT (`ONLINE`, `OFF rc=N`, `NO WIFI`)
  - `DEV : x/2`
  - Mode (`ARMED`/`DISARMED`)
  - Status pintu/alarm (`ONLINE`, `OFFLINE`, `NO DATA`)
  - Jumlah peer ESP-NOW (`0/2`, `1/2`, `2/2 PEER`)
- `NO DATA` berarti belum pernah menerima data sejak hub boot; `OFFLINE` berarti heartbeat terakhir lebih dari 90 detik.
- Status: berfungsi; terkonfirmasi dari log boot `[GW] OLED siap`.

### Kasus 7 — Pin buzzer/relay
- Output alarm menggerakkan:
  - GPIO 5 (buzzer utama)
  - GPIO 0 dan GPIO 1 (kompatibilitas wiring board alarm lama)
  - GPIO 6 (relay)
  - GPIO 8 (LED status)
- Level aktif adalah `HIGH` (`BUZZER_ACTIVE_LEVEL`, `RELAY_ACTIVE_LEVEL` di `common/config.h`).
- Pengujian MQTT memastikan `alarm/state` berubah `OFF -> ON -> OFF`.
- Konfirmasi suara buzzer secara fisik masih diperlukan; jika tidak berbunyi, periksa wiring active-low atau balik konstanta level aktif.
- Status: output terverifikasi, suara belum dikonfirmasi lewat software.

### Kasus 8 — Input switch pintu
- Node pintu membaca GPIO 4 (default AGENTS) dan GPIO 3 (wiring board tombol lama).
- `INPUT_PULLUP`, level aktif `LOW`, debounce 50 ms.
- Polaritas switch dibalik dengan `DOOR_INVERT=true` di `common/config.h`; kondisi idle sekarang dilaporkan `CLOSED`, dan event/status mengikuti logika baru.
- Terverifikasi lewat MQTT: `door/state` berubah dari `OPEN` menjadi `CLOSED` setelah firmware baru di-upload.
- Jika wiring fisik ternyata berkebalikan, ubah `DOOR_INVERT` menjadi `false`.
- Status: berfungsi; toggle fisik tetap disarankan untuk memastikan transisi.

### Kasus 9 — Durasi alarm 3 menit
- Alarm menyala selama **3 menit** setelah trigger (`ALARM_ON_DURATION_MS = 180000` di `common/config.h`).
- Timer berjalan di node alarm: setelah 3 menit, relay/buzzer dimatikan dan node mengirim heartbeat `value=0` supaya gateway segera mempublikasikan `alarm/state OFF`.
- Status: terverifikasi lewat MQTT; `alarm/state` menjadi `ON` pada detik 6 dan kembali `OFF` otomatis pada detik 186 (180 detik setelah ON).

## 3. Timeline (perkiraan, dari log/timestamp file)

| Waktu | Kejadian |
|---|---|
| 13:39–14:05 | Dashboard MQTT + Dexie diimplementasikan dan diuji dengan pesan simulasi |
| 15:04 | `common/packet.h` dibuat; pengerjaan firmware ESP-NOW dimulai |
| 15:30–16:00 | Firmware pintu/alarm/gateway diimplementasikan; semua platform berhasil compile |
| 16:10 | Instruksi setup `README.md` selesai |
| 16:17 | `AGENTS.md` diperbarui dengan desain ESP-NOW yang diimplementasikan |
| 16:20–16:45 | Hub (COM3) dan pintu (COM4) di-flash; pintu muncul `TX FAIL` |
| 16:45–17:10 | Diagnosa: key, channel, callback, tes tanpa enkripsi, receiver kedua, hub khusus ESP-NOW |
| 17:10–17:20 | Akar masalah ditemukan: TX 19.5 dBm tidak stabil; diganti ke 8.5 dBm |
| 17:21 | Pintu + alarm online; `DEV : 2/2`; ARM → ON, DISARM → OFF terverifikasi |
| 17:22 | Ringkasan dibuat |

## 4. Build dan perintah

Ketiga environment berhasil compile dan upload:

```text
deviceSensorDoor  flash ~54.6%  RAM ~9.2%
deviceAlert       flash ~54.6%  RAM ~9.2%
deviceHub         flash ~70.3%  RAM ~10.0%
```

Perintah upload:

```bash
pio run -d deviceHub        -t upload --upload-port COM3
pio run -d deviceSensorDoor -t upload --upload-port COM4
pio run -d deviceAlert      -t upload --upload-port COM5
```

Serial monitor (membuka port akan mereset board):

```bash
pio device monitor --port COM3 --baud 115200
```

## 5. Catatan konfigurasi

`common/secrets.h` (masuk gitignore) saat ini berisi:
- Wi-Fi: `SmartHub` / password kosong
- MQTT: `broker.emqx.io:8883`, TLS, anonymous
- CA: DigiCert Global Root G2
- PMK acak, LMK pintu, LMK alarm
- MAC hub, pintu, dan alarm

`common/config.h`:
- Pintu: GPIO 4 + GPIO 3, `INPUT_PULLUP`, `DOOR_INVERT=true` (idle = CLOSED)
- Alarm: buzzer GPIO 5/0/1, relay GPIO 6, LED GPIO 8
- OLED gateway: SDA 6, SCL 7, `0x3C`
- Heartbeat 30 detik, ambang offline 90 detik
- Daya TX: 8.5 dBm (`espnowBegin`), sama dengan firmware IoT lama yang bekerja

Environment dashboard:
```text
APP_MQTT_HOST=wss://broker.emqx.io:8084/mqtt
APP_MQTT_TOPIC_PREFIXES=1003250005,kel1/wsn
```

## 6. Keterbatasan yang diketahui / langkah berikutnya

1. Pastikan buzzer alarm berbunyi saat `alarm/state` bernilai `ON`. Jika tidak, cek GPIO yang dipakai buzzer dan apakah aktif-low.
2. Toggle switch pintu dan pastikan `door/state` berubah serta OLED ikut memperbarui.
3. Broker MQTT bersifat publik dan tanpa autentikasi; siapa pun bisa subscribe ke `kel1/wsn/...`. TLS hanya melindungi transport.
4. Hanya gateway yang memiliki kredensial Wi-Fi/MQTT; pintu dan alarm murni ESP-NOW.
5. ESP-NOW membutuhkan semua node di channel 6. Jaga `ESPNOW_CHANNEL=6` agar sama dengan AP SmartHub.
6. Tidak ada jalur update OTA; flashing dilakukan lewat USB.

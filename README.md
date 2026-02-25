# 🕌 Auto Tarhim & Bel Elektrik IoT (ESP8266)

[cite_start]Proyek *Internet of Things* (IoT) berbasis mikrokontroler NodeMCU ESP8266 untuk mengotomatisasi pemutaran audio Tarhim sebelum waktu salat dan membunyikan Bel Elektrik sesuai jadwal[cite: 109, 114]. [cite_start]Sistem ini dilengkapi dengan Web Dashboard UI yang modern dan responsif untuk memudahkan pengaturan tanpa harus mengubah kode program[cite: 139, 140, 141].

## 📖 Deskripsi

[cite_start]Alat ini mengambil data jadwal salat secara *real-time* dari API MyQuran. [cite_start]Menjelang waktu salat (bisa diatur berapa menit sebelumnya), alat akan otomatis menyalakan Amplifier (via Relay) dan memutar file MP3 Tarhim dari modul DFPlayer Mini[cite: 111, 115, 209]. [cite_start]Selain itu, terdapat fitur Bel Elektrik otomatis yang sangat cocok untuk jadwal masuk sekolah atau kegiatan pondok pesantren[cite: 115]. 

[cite_start]Semua pengaturan disimpan di dalam memori internal ESP8266 menggunakan sistem *LittleFS*, sehingga data tidak akan hilang ketika listrik padam[cite: 109, 121].

## ✨ Fitur Utama

- [cite_start]**📡 Sinkronisasi Waktu & Jadwal Otomatis**: Mendapatkan waktu via NTP Server dan jadwal salat harian dari API MyQuran.
- [cite_start]**📻 Auto Tarhim**: Mengaktifkan relay amplifier dan memutar audio dari SD Card secara otomatis sebelum adzan[cite: 224, 228]. [cite_start]Pilihan file MP3 dan durasi hitung mundur dapat dikustomisasi[cite: 183, 186].
- [cite_start]**🔔 Auto Bel Elektrik**: Menyediakan hingga 15 slot jadwal bel elektrik[cite: 118]. [cite_start]Bisa diatur waktu berbunyi, jumlah dering, dan durasi per dering[cite: 192, 193, 194].
- [cite_start]**💻 Web Dashboard (Dark Mode)**: Antarmuka web yang modern, responsif, dan mendukung *tabbing* (Tab Tarhim & Tab Bel) untuk kemudahan manajemen[cite: 141, 178].
- [cite_start]**📱 Remote Manual**: Tombol *bypass* di web UI untuk menyalakan/mematikan Ampli secara manual, serta tombol "Tahan" untuk membunyikan Bel secara manual dengan fitur *Failsafe* (anti-hang)[cite: 236, 237, 238].
- **📺 LCD 16x2 dengan 3 Mode**: Tombol fisik untuk mengganti tampilan layar LCD: 
  - [cite_start]Mode 0: Jam & Hitung Mundur Tarhim[cite: 266, 270].
  - [cite_start]Mode 1: Jam & Hitung Mundur Bel Selanjutnya[cite: 250, 252].
  - [cite_start]Mode 2: Informasi Alamat IP[cite: 240].

## 🛠️ Kebutuhan Hardware

1. [cite_start]NodeMCU ESP8266 (atau Wemos D1 Mini)[cite: 109].
2. [cite_start]Modul DFPlayer Mini + MicroSD Card (berisi file MP3)[cite: 109, 111].
3. [cite_start]Modul Relay 2 Channel (Active HIGH)[cite: 114, 115, 209].
4. [cite_start]LCD 16x2 + Modul I2C[cite: 111].
5. [cite_start]Tombol Push Button (Tactile Switch)[cite: 115].
6. [cite_start]Amplifier & Speaker (Terkoneksi ke Relay 1)[cite: 115].
7. [cite_start]Bel Elektrik (Terkoneksi ke Relay 2)[cite: 115].

## 📌 Skema Pin (Wiring)

Berikut adalah pemetaan pin berdasarkan kode program:

| Komponen | Pin Modul | Pin NodeMCU ESP8266 | Keterangan |
| :--- | :--- | :--- | :--- |
| **DFPlayer Mini** | RX | `D1` | [cite_start]Menggunakan SoftwareSerial [cite: 111] |
| | TX | `D2` | [cite_start]Menggunakan SoftwareSerial [cite: 111] |
| **LCD 16x2 I2C** | SDA | `D6` | [cite_start]Dideklarasikan via `Wire.begin(D6, D5)` [cite: 212] |
| | SCL | `D5` | [cite_start]Dideklarasikan via `Wire.begin(D6, D5)` [cite: 212] |
| **Relay Ampli** | IN 1 | `D8` | [cite_start]Active HIGH [cite: 114, 209] |
| **Relay Bel** | IN 2 | `D7` | [cite_start]Active HIGH [cite: 115, 234] |
| **Tombol LCD** | Kaki 1 | `D4` | [cite_start]Menggunakan internal PULLUP (`INPUT_PULLUP`) [cite: 115, 212] |
| | Kaki 2 | `GND` | [cite_start]Ditekan menyambung ke Ground [cite: 220] |

*Catatan: Pastikan VCC dan GND setiap komponen terhubung dengan baik. Untuk DFPlayer, sangat disarankan menggunakan resistor 1K pada jalur RX untuk menghilangkan noise (mendesis).*

## 📚 Library yang Dibutuhkan

[cite_start]Pastikan Anda telah menginstal pustaka (*library*) berikut melalui **Arduino Library Manager** sebelum melakukan kompilasi[cite: 109]:
- [cite_start]`ESP8266WiFi`, `ESP8266WebServer`, `ESP8266HTTPClient` (Bawaan core ESP8266)[cite: 109].
- [cite_start]`NTPClient` (oleh Fabrice Weinberg)[cite: 109].
- [cite_start]`LiquidCrystal_I2C` (oleh Frank de Brabander)[cite: 109].
- [cite_start]`DFRobotDFPlayerMini` (oleh DFRobot)[cite: 109].
- [cite_start]`ArduinoJson` (oleh Benoit Blanchon - **Gunakan Versi 6.x**)[cite: 109, 121].

## 🚀 Cara Instalasi

1. Unduh atau *clone repository* ini.
2. Buka file `.ino` menggunakan **Arduino IDE**.
3. Ubah konfigurasi WiFi pada baris kode berikut agar sesuai dengan jaringan Anda:
   ```cpp
   const char* ssid     = "NAMA_WIFI_ANDA";
   const char* password = "PASSWORD_WIFI_ANDA";
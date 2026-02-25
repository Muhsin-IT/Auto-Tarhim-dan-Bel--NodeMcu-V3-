# 🕌 Auto Tarhim & Bel Elektrik IoT (ESP8266)
Proyek *Internet of Things* (IoT) berbasis mikrokontroler NodeMCU ESP8266 untuk mengotomatisasi pemutaran audio Tarhim sebelum waktu salat dan membunyikan Bel Elektrik sesuai jadwal[cite: 109, 114].Sistem ini dilengkapi dengan Web Dashboard UI yang modern dan responsif untuk memudahkan pengaturan tanpa harus mengubah kode program[cite: 139, 140, 141].

## 📖 Deskripsi
Alat ini mengambil data jadwal salat secara *real-time* dari API MyQuran.Menjelang waktu salat (bisa diatur berapa menit sebelumnya), alat akan otomatis menyalakan Amplifier (via Relay) dan memutar file MP3 Tarhim dari modul DFPlayer Mini[cite: 111, 115, 209].Selain itu, terdapat fitur Bel Elektrik otomatis yang sangat cocok untuk jadwal masuk sekolah atau kegiatan pondok pesantren[cite: 115]. 
Semua pengaturan disimpan di dalam memori internal ESP8266 menggunakan sistem *LittleFS*, sehingga data tidak akan hilang ketika listrik padam[cite: 109, 121].

## ✨ Fitur Utama

-**📡 Sinkronisasi Waktu & Jadwal Otomatis**: Mendapatkan waktu via NTP Server dan jadwal salat harian dari API MyQuran.
-**📻 Auto Tarhim**: Mengaktifkan relay amplifier dan memutar audio dari SD Card secara otomatis sebelum adzan[cite: 224, 228].Pilihan file MP3 dan durasi hitung mundur dapat dikustomisasi[cite: 183, 186].
-**🔔 Auto Bel Elektrik**: Menyediakan hingga 15 slot jadwal bel elektrik[cite: 118].Bisa diatur waktu berbunyi, jumlah dering, dan durasi per dering[cite: 192, 193, 194].
-**💻 Web Dashboard (Dark Mode)**: Antarmuka web yang modern, responsif, dan mendukung *tabbing* (Tab Tarhim & Tab Bel) untuk kemudahan manajemen[cite: 141, 178].
-**📱 Remote Manual**: Tombol *bypass* di web UI untuk menyalakan/mematikan Ampli secara manual, serta tombol "Tahan" untuk membunyikan Bel secara manual dengan fitur *Failsafe* (anti-hang)[cite: 236, 237, 238].
- **📺 LCD 16x2 dengan 3 Mode**: Tombol fisik untuk mengganti tampilan layar LCD: 
  -Mode 0: Jam & Hitung Mundur Tarhim[cite: 266, 270].
  -Mode 1: Jam & Hitung Mundur Bel Selanjutnya[cite: 250, 252].
  -Mode 2: Informasi Alamat IP[cite: 240].

## 🛠️ Kebutuhan Hardware

1.NodeMCU ESP8266 (atau Wemos D1 Mini)[cite: 109].
2.Modul DFPlayer Mini + MicroSD Card (berisi file MP3)[cite: 109, 111].
3.Modul Relay 2 Channel (Active HIGH)[cite: 114, 115, 209].
4.LCD 16x2 + Modul I2C[cite: 111].
5.Tombol Push Button (Tactile Switch)[cite: 115].
6.Amplifier & Speaker (Terkoneksi ke Relay 1)[cite: 115].
7.Bel Elektrik (Terkoneksi ke Relay 2)[cite: 115].

## 📌 Skema Pin (Wiring)

Berikut adalah pemetaan pin berdasarkan kode program:

| Komponen | Pin Modul | Pin NodeMCU ESP8266 | Keterangan |
| :--- | :--- | :--- | :--- |
| **DFPlayer Mini** | RX | `D1` |Menggunakan SoftwareSerial [cite: 111] |
| | TX | `D2` |Menggunakan SoftwareSerial [cite: 111] |
| **LCD 16x2 I2C** | SDA | `D6` |Dideklarasikan via `Wire.begin(D6, D5)` [cite: 212] |
| | SCL | `D5` |Dideklarasikan via `Wire.begin(D6, D5)` [cite: 212] |
| **Relay Ampli** | IN 1 | `D8` |Active HIGH [cite: 114, 209] |
| **Relay Bel** | IN 2 | `D7` |Active HIGH [cite: 115, 234] |
| **Tombol LCD** | Kaki 1 | `D4` |Menggunakan internal PULLUP (`INPUT_PULLUP`) [cite: 115, 212] |
| | Kaki 2 | `GND` |Ditekan menyambung ke Ground [cite: 220] |

*Catatan: Pastikan VCC dan GND setiap komponen terhubung dengan baik. Untuk DFPlayer, sangat disarankan menggunakan resistor 1K pada jalur RX untuk menghilangkan noise (mendesis).*

## 📚 Library yang Dibutuhkan
Pastikan Anda telah menginstal pustaka (*library*) berikut melalui **Arduino Library Manager** sebelum melakukan kompilasi[cite: 109]:
-`ESP8266WiFi`, `ESP8266WebServer`, `ESP8266HTTPClient` (Bawaan core ESP8266)[cite: 109].
-`NTPClient` (oleh Fabrice Weinberg)[cite: 109].
-`LiquidCrystal_I2C` (oleh Frank de Brabander)[cite: 109].
-`DFRobotDFPlayerMini` (oleh DFRobot)[cite: 109].
-`ArduinoJson` (oleh Benoit Blanchon - **Gunakan Versi 6.x**)[cite: 109, 121].

## 🚀 Cara Instalasi

1. Unduh atau *clone repository* ini.
2. Buka file `.ino` menggunakan **Arduino IDE**.
3. Ubah konfigurasi WiFi pada baris kode berikut agar sesuai dengan jaringan Anda:
   ```cpp
   const char* ssid     = "NAMA_WIFI_ANDA";
   const char* password = "PASSWORD_WIFI_ANDA";
4. isi file audio pada kartu memory , masukan file audio di dalam folder **mp3** dan penaaan fila audio dengan format 0001.mp3 dst
    struktur folder seperti ini
   
    SD Card/
    └── mp3/
        ├── 0001.mp3
        ├── 0002.mp3
        └── 0003.mp3
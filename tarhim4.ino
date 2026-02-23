#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LittleFS.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "Kantor Pusat✋😎🤚";
const char* password = "kantorpusat1973";

// --- KONFIGURASI PIN ---
LiquidCrystal_I2C lcd(0x27, 16, 2); 
SoftwareSerial mySoftwareSerial(D1, D2); 
DFRobotDFPlayerMini myDFPlayer;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 7 * 3600);
ESP8266WebServer server(80);

// --- VARIABEL SISTEM TARHIM ---
String idKota = "1501";
String vImsak="--:--", vSubuh="--:--", vTerbit="--:--", vDzuhur="--:--", vAshar="--:--", vMaghrib="--:--", vIsya="--:--";
int totalFiles = 0;
int volumeAlat = 15;
int lcdMode = 0; 

int pinRelay = D8;       // Relay Ampli (Tarhim)
int pinRelayBel = D7;    // Relay Bel Elektrik
int pinTombol = D4;      // Tombol LCD

// Variabel Kontrol Audio Tarhim
int currentFilePlaying = 0;
bool isPlaying = false, isPaused = false;
unsigned long lastTriggerTime = 0;
unsigned long playStartTime = 0;

bool aktifTarhim[7] = {true, true, false, false, false, true, false}; 
int fileSesi[7] = {3, 1, 1, 1, 1, 1, 1};
int durasiFiles[31];

// --- VARIABEL SISTEM BEL ---
String bWaktu[15];
int bKali[15];
int bDurasi[15];
bool bAktif[15];

bool isBelRinging = false;
int belRingsLeft = 0;
int currentBelDuration = 0;
unsigned long lastBelToggleTime = 0;
bool belRelayState = false;

// --- VARIABEL KONTROL MANUAL (REMOTE UI) ---
bool manualAmpli = false;
bool manualBel = false;
unsigned long lastBelPing = 0; // Untuk proteksi Failsafe

void saveConfig() {
  File f = LittleFS.open("/config.json", "w");
  if (!f) return;
  DynamicJsonDocument doc(4096); 
  doc["kota"] = idKota; doc["vol"] = volumeAlat;
  
  JsonArray actArr = doc.createNestedArray("aktif");
  for(int i=0; i<7; i++) actArr.add(aktifTarhim[i]);
  JsonArray fArr = doc.createNestedArray("filesesi");
  for(int i=0; i<7; i++) fArr.add(fileSesi[i]);
  JsonArray durArray = doc.createNestedArray("durasi");
  for(int i=1; i<=30; i++) durArray.add(durasiFiles[i]);
  
  JsonArray baArr = doc.createNestedArray("bA");
  JsonArray bwArr = doc.createNestedArray("bW");
  JsonArray bkArr = doc.createNestedArray("bK");
  JsonArray bdArr = doc.createNestedArray("bD");
  for(int i=0; i<15; i++) {
    baArr.add(bAktif[i]); bwArr.add(bWaktu[i]); bkArr.add(bKali[i]); bdArr.add(bDurasi[i]);
  }
  
  serializeJson(doc, f);
  f.close();
}

void loadConfig() {
  for(int i=0; i<15; i++) { bWaktu[i]="--:--"; bKali[i]=1; bDurasi[i]=3; bAktif[i]=false; }
  if (!LittleFS.exists("/config.json")) return;
  File f = LittleFS.open("/config.json", "r");
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, f);
  
  idKota = doc["kota"].as<String>();
  volumeAlat = doc["vol"] | 15;
  for(int i=0; i<7; i++) {
    aktifTarhim[i] = doc["aktif"][i] | false;
    fileSesi[i] = doc["filesesi"][i] | 1;
  }
  for(int i=1; i<=30; i++) durasiFiles[i] = doc["durasi"][i-1] | 0;
  
  for(int i=0; i<15; i++) {
    bAktif[i] = doc["bA"][i] | false;
    if (doc["bW"][i]) bWaktu[i] = doc["bW"][i].as<String>();
    bKali[i] = doc["bK"][i] | 1;
    bDurasi[i] = doc["bD"][i] | 3;
  }
  f.close();
}

void fetchJadwal() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    timeClient.update();
    time_t rawtime = timeClient.getEpochTime();
    struct tm * ti = localtime (&rawtime);
    char tgl[15];
    sprintf(tgl, "%04d/%02d/%02d", ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday);
    String url = "https://api.myquran.com/v2/sholat/jadwal/" + idKota + "/" + String(tgl); 
    if (http.begin(client, url)) {
      if (http.GET() > 0) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getString());
        JsonObject j = doc["data"]["jadwal"];
        vImsak = j["imsak"].as<String>(); vSubuh = j["subuh"].as<String>();
        vTerbit = j["terbit"].as<String>(); vDzuhur = j["dzuhur"].as<String>();
        vAshar = j["ashar"].as<String>(); vMaghrib = j["maghrib"].as<String>();
        vIsya = j["isya"].as<String>();
      }
      http.end();
    }
  }
}

String getFileOptions(int selected) {
  String opt = "";
  for(int i=1; i<=totalFiles; i++) { 
    opt += "<option value='" + String(i) + "'" + (i==selected?" selected":"") + ">File " + String(i) + "</option>";
  }
  return opt;
}

void handleRoot() {
  String s = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>";
  s += "<title>Tarhim & Bel Dashboard</title>";
  s += "<link rel='icon' type='image/png' sizes='32x32' href='https://img.almiftah.online/assets/Icon.png'>";
  s += "<style>";
  s += ":root { --primary: #38bdf8; --success: #22c55e; --warning: #facc15; --danger: #ef4444; --bg: #0f172a; --card: #1e293b; --text: #f8fafc; --text-muted: #94a3b8; --border: #334155; }";
  s += "body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--text); padding: 15px; margin: 0; display: flex; justify-content: center; user-select: none; }";
  s += ".card { background: var(--card); padding: 25px; border-radius: 16px; box-shadow: 0 10px 25px -5px rgba(0,0,0,0.5); width: 100%; max-width: 600px; box-sizing: border-box; }";
  s += "h2 { text-align: center; color: var(--primary); margin-top: 0; font-size: 1.5rem; letter-spacing: 0.5px; }";
  s += "h4 { color: var(--text); border-bottom: 2px solid var(--border); padding-bottom: 8px; margin-top: 20px; }";
  s += ".grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }";
  s += ".input-group { display: flex; flex-direction: column; }";
  s += "label.title { font-size: 12px; font-weight: 600; margin-bottom: 6px; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px; display: flex; justify-content: space-between; }";
  s += "input[type=number], input[type=time], input[type=text], select { padding: 10px 12px; border: 1px solid var(--border); border-radius: 8px; font-size: 14px; width: 100%; box-sizing: border-box; outline: none; transition: border 0.2s; background: var(--bg); color: var(--text); }";
  s += "input:focus, select:focus { border-color: var(--primary); }";
  s += "input[type=range] { width: 100%; accent-color: var(--primary); margin-top: 5px; cursor: pointer; }";
  s += "table { width: 100%; border-collapse: collapse; margin-bottom: 20px; font-size: 14px; text-align: left; }";
  s += "th { background: rgba(0,0,0,0.2); padding: 12px 10px; border-bottom: 2px solid var(--border); color: var(--text-muted); font-weight: 600; white-space:nowrap; }";
  s += "td { padding: 12px 10px; border-bottom: 1px solid var(--border); vertical-align: middle; }";
  s += ".td-center { text-align: center; }";
  s += ".btn { border: none; padding: 10px 15px; border-radius: 8px; cursor: pointer; font-weight: 600; color: #0f172a; transition: all 0.2s ease; text-decoration: none; display: inline-block; text-align: center; box-sizing: border-box; }";
  s += ".b-primary { background: var(--primary); width: 100%; font-size: 15px; padding: 14px; margin-top: 10px; color: #0f172a; box-shadow: 0 4px 6px -1px rgba(56, 189, 248, 0.2); }";
  s += ".b-play { background: var(--success); padding: 8px 12px; font-size: 12px; width: 100%; margin-top: 8px; color: #0f172a; }";
  s += ".b-pause { background: var(--warning); padding: 8px; font-size: 12px; flex: 1; margin: 0; color: #0f172a; }";
  s += ".b-stop { background: var(--danger); padding: 8px; font-size: 12px; flex: 1; margin: 0; color: #fff; }";
  s += ".btn:hover { filter: brightness(1.1); }";
  s += ".btn:active { transform: scale(0.97); }";
  s += ".btn-group { display: flex; gap: 5px; width: 100%; margin-top: 8px; }";
  s += ".durasi-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(130px, 1fr)); gap: 12px; }";
  s += ".durasi-item { background: rgba(0,0,0,0.15); border: 1px solid var(--border); padding: 15px; border-radius: 12px; display: flex; flex-direction: column; align-items: center; }";
  s += ".durasi-item span { margin-bottom: 8px; color: var(--text); font-weight: 700; font-size: 14px; }";
  s += ".durasi-item input { text-align: center; }";
  s += ".switch { position: relative; display: inline-block; width: 44px; height: 24px; }";
  s += ".switch input { opacity: 0; width: 0; height: 0; }";
  s += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #475569; transition: .3s; border-radius: 24px; }";
  s += ".slider:before { position: absolute; content: ''; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; box-shadow: 0 2px 4px rgba(0,0,0,0.3); }";
  s += "input:checked + .slider { background-color: var(--success); }";
  s += "input:checked + .slider:before { transform: translateX(20px); }";
  s += ".tabs { display: flex; justify-content: center; gap: 10px; border-bottom: 2px solid var(--border); margin-bottom: 20px; }";
  s += ".tab-btn { background: none; border: none; padding: 10px 20px; color: var(--text-muted); font-size: 15px; font-weight: 600; cursor: pointer; transition: 0.3s; border-bottom: 2px solid transparent; margin-bottom: -2px; }";
  s += ".tab-btn.active { color: var(--primary); border-bottom: 2px solid var(--primary); }";
  s += ".tab-content { display: none; animation: fadeEffect 0.3s; }";
  s += ".tab-content.active { display: block; }";
  s += "@keyframes fadeEffect { from {opacity: 0;} to {opacity: 1;} }";
  s += ".footer { text-align: center; font-size: 12px; color: var(--text-muted); margin-top: 30px; border-top: 1px solid var(--border); padding-top: 15px; }";
  s += "</style></head><body>";
  
  // Data State tersembunyi
  s += "<div id='s-data' data-play='" + String(isPlaying ? 1 : 0) + "' data-pause='" + String(isPaused ? 1 : 0) + "' data-file='" + String(currentFilePlaying) + "' data-ampli='" + String(manualAmpli ? 1 : 0) + "' style='display:none;'></div>";

  s += "<div class='card'>";
  s += "<h2>Auto Tarhim & Bel</h2>";
  
  // --- KONTROL MANUAL REMOTE ---
  s += "<div style='display:flex; gap:10px; margin-bottom:20px; margin-top:15px;'>";
  s += "<button id='btn-ampli' type='button' class='btn' style='flex:1; background: " + String(manualAmpli ? "var(--success)" : "var(--border)") + "; color: #fff;' onclick='toggleAmpli()'>Ampli: " + String(manualAmpli ? "ON" : "OFF") + "</button>";
  s += "<button id='btn-bel' type='button' class='btn' style='flex:1; background: var(--primary); color: #0f172a;'>🔔 Tahan Bel</button>";
  s += "</div>";

  s += "<form action='/save'>";
  s += "<div class='grid-2'>";
  s += "<div class='input-group'><label class='title'>ID Kota</label><input type='text' name='kota' value='" + idKota + "'></div>";
  s += "<div class='input-group'><label class='title'><span>Volume Tarhim</span><span id='vol-val' style='color:var(--primary);'>" + String(volumeAlat) + "</span></label>";
  s += "<input type='range' name='vol' min='0' max='30' value='" + String(volumeAlat) + "' oninput='document.getElementById(\"vol-val\").innerText=this.value'></div>";
  s += "</div>";

  s += "<div class='tabs'>";
  s += "<button type='button' class='tab-btn active' onclick=\"openTab(event, 'Tarhim')\">Tarhim Set</button>";
  s += "<button type='button' class='tab-btn' onclick=\"openTab(event, 'Bel')\">Bel Set</button>";
  s += "</div>";
  
  // TAB TARHIM
  s += "<div id='Tarhim' class='tab-content active'>";
  s += "<div style='overflow-x:auto;'><table><tr><th>Jadwal</th><th>Jam</th><th>Aktif</th><th>Pilih File</th></tr>";
  String names[] = {"Imsak", "Subuh", "Terbit", "Dzuhur", "Ashar", "Maghrib", "Isya"};
  String vals[] = {vImsak, vSubuh, vTerbit, vDzuhur, vAshar, vMaghrib, vIsya};
  
  for(int i = 0; i < 7; i++) {
    s += "<tr><td><b>" + names[i] + "</b></td><td>" + vals[i] + "</td>";
    s += "<td><label class='switch'><input type='checkbox' name='a" + String(i) + "' " + (aktifTarhim[i] ? "checked" : "") + "><span class='slider'></span></label></td>";
    s += "<td><select name='f" + String(i) + "'>" + getFileOptions(fileSesi[i]) + "</select></td></tr>";
  }
  s += "</table></div>";
  s += "<h4>Set Durasi Audio</h4>";
  s += "<p style='font-size:12px; color:var(--text-muted); margin-top:-5px;'>satuan detik</p>";
  s += "<div class='durasi-grid'>";
  for(int i = 1; i <= totalFiles; i++){
    s += "<div class='durasi-item'><span>Audio " + String(i) + "</span>";
    s += "<input type='number' name='d" + String(i) + "' value='" + String(durasiFiles[i]) + "' placeholder='Detik'>";
    s += "<div id='btn-" + String(i) + "' style='width:100%;'>";
    if (isPlaying && currentFilePlaying == i) {
      s += "<div class='btn-group'><a href='/pause' class='btn b-pause'>" + String(isPaused ? "Resume" : "Pause") + "</a><a href='/stop' class='btn b-stop'>Stop</a></div>";
    } else {
      s += "<a href='/playtest?n=" + String(i) + "' class='btn b-play'>Play</a>";
    }
    s += "</div></div>";
  }
  s += "</div></div>";

  // TAB BEL
  s += "<div id='Bel' class='tab-content'>";
  s += "<div style='overflow-x:auto;'><table><tr><th>Aktif</th><th>Jam</th><th class='td-center'>X Bunyi</th><th class='td-center'>Lama (Detik)</th></tr>";
  for(int i = 0; i < 15; i++) {
    s += "<tr>";
    s += "<td><label class='switch'><input type='checkbox' name='bA" + String(i) + "' " + (bAktif[i] ? "checked" : "") + "><span class='slider'></span></label></td>";
    s += "<td><input type='time' name='bW" + String(i) + "' value='" + bWaktu[i] + "'></td>";
    s += "<td><input type='number' name='bK" + String(i) + "' value='" + String(bKali[i]) + "' min='1' max='20' style='text-align:center;'></td>";
    s += "<td><input type='number' name='bD" + String(i) + "' value='" + String(bDurasi[i]) + "' min='1' max='60' style='text-align:center;'></td>";
    s += "</tr>";
  }
  s += "</table></div></div>";
  
  s += "<br><button type='submit' class='btn b-primary'>SIMPAN SEMUA PENGATURAN</button></form>";
  s += "<div class='footer'>Developed &#9829; by <b style='color:var(--primary);'>Muhsin-IT &#169;</b><br>Waktu Perangkat: <b id='jam' style='color:var(--primary);'>" + timeClient.getFormattedTime() + "</b></div>";
  s += "</div>";

  // --- JAVASCRIPT ---
  s += "<script>";
  s += "function openTab(evt, tabName) {";
  s += "  let i, tabcontent, tablinks;";
  s += "  tabcontent = document.getElementsByClassName('tab-content');";
  s += "  for (i = 0; i < tabcontent.length; i++) { tabcontent[i].classList.remove('active'); }";
  s += "  tablinks = document.getElementsByClassName('tab-btn');";
  s += "  for (i = 0; i < tablinks.length; i++) { tablinks[i].classList.remove('active'); }";
  s += "  document.getElementById(tabName).classList.add('active');";
  s += "  if(evt) evt.currentTarget.classList.add('active');";
  s += "  localStorage.setItem('activeTab', tabName);";
  s += "}";
  
  s += "document.addEventListener('DOMContentLoaded', () => {";
  s += "  let active = localStorage.getItem('activeTab');";
  s += "  if(active) { let btn = document.querySelector(`.tab-btn[onclick*=\"'${active}'\"]`); if(btn) btn.click(); }";
  s += "});";

  // JS Kontrol Manual Ampli
  s += "let ampliOn = " + String(manualAmpli ? "true" : "false") + ";";
  s += "function toggleAmpli() {";
  s += "  ampliOn = !ampliOn;";
  s += "  let btn = document.getElementById('btn-ampli');";
  s += "  btn.innerText = 'Ampli: ' + (ampliOn ? 'ON' : 'OFF');";
  s += "  btn.style.background = ampliOn ? 'var(--success)' : 'var(--border)';";
  s += "  fetch('/ampli?state=' + (ampliOn ? 1 : 0));";
  s += "}";

  // JS Kontrol Manual Bel (Tahan / Heartbeat)
  s += "let belPingTimer;";
  s += "const btnBel = document.getElementById('btn-bel');";
  s += "function startBel(e) {";
  s += "  if(e && e.cancelable) e.preventDefault();"; // Hindari scrolling di HP
  s += "  fetch('/bel?state=1');";
  s += "  belPingTimer = setInterval(()=>fetch('/bel?state=1'), 800);"; // Ping tiap 800ms
  s += "  btnBel.style.background = 'var(--danger)';";
  s += "  btnBel.style.color = '#fff';";
  s += "}";
  s += "function stopBel(e) {";
  s += "  if(e && e.cancelable) e.preventDefault();";
  s += "  clearInterval(belPingTimer);";
  s += "  fetch('/bel?state=0');";
  s += "  btnBel.style.background = 'var(--primary)';";
  s += "  btnBel.style.color = '#0f172a';";
  s += "}";
  
  // Event Listeners untuk Touch HP dan Mouse PC
  s += "btnBel.addEventListener('mousedown', startBel);";
  s += "btnBel.addEventListener('mouseup', stopBel);";
  s += "btnBel.addEventListener('mouseleave', stopBel);"; // Jika jari/mouse geser keluar tombol
  s += "btnBel.addEventListener('touchstart', startBel, {passive: false});";
  s += "btnBel.addEventListener('touchend', stopBel, {passive: false});";
  s += "btnBel.addEventListener('touchcancel', stopBel, {passive: false});";

  // JS Jam Berjalan
  s += "let tf = " + String(totalFiles) + ";";
  s += "let timeStr = '" + timeClient.getFormattedTime() + "';";
  s += "let [h, m, sec] = timeStr.split(':').map(Number);";
  s += "setInterval(function() {";
  s += "  sec++; if(sec > 59) { sec = 0; m++; } if(m > 59) { m = 0; h++; } if(h > 23) { h = 0; }";
  s += "  document.getElementById('jam').innerText = String(h).padStart(2,'0') + ':' + String(m).padStart(2,'0') + ':' + String(sec).padStart(2,'0');";
  s += "}, 1000);";

  // JS Sinkronisasi Status (AJAX Polling)
  s += "setInterval(function() {";
  s += "  fetch('/').then(function(res) { return res.text(); }).then(function(html) {";
  s += "    let parser = new DOMParser().parseFromString(html, 'text/html');";
  s += "    let sData = parser.getElementById('s-data');";
  s += "    if(sData) {";
  s += "      let isPlay = sData.dataset.play === '1';";
  s += "      let isPause = sData.dataset.pause === '1';";
  s += "      let curFile = sData.dataset.file;";
  
  // Sinkronisasi status tombol Ampli
  s += "      let isAmpli = sData.dataset.ampli === '1';";
  s += "      ampliOn = isAmpli;";
  s += "      let btnA = document.getElementById('btn-ampli');";
  s += "      if(btnA) {";
  s += "        btnA.innerText = 'Ampli: ' + (ampliOn ? 'ON' : 'OFF');";
  s += "        btnA.style.background = ampliOn ? 'var(--success)' : 'var(--border)';";
  s += "      }";

  s += "      for(let i=1; i<=tf; i++) {";
  s += "        let btnDiv = document.getElementById('btn-' + i);";
  s += "        if(btnDiv) {";
  s += "          if(isPlay && curFile == i) {";
  s += "            btnDiv.innerHTML = \"<div class='btn-group'><a href='/pause' class='btn b-pause'>\" + (isPause ? 'Resume' : 'Pause') + \"</a><a href='/stop' class='btn b-stop'>Stop</a></div>\";";
  s += "          } else {";
  s += "            btnDiv.innerHTML = \"<a href='/playtest?n=\" + i + \"' class='btn b-play'>Play</a>\";";
  s += "          }";
  s += "        }";
  s += "      }";
  s += "    }";
  s += "  }).catch(function(err){});";
  s += "}, 3000);";
  s += "</script></body></html>";
  
  server.send(200, "text/html", s);
}

void playAudio(int f) {
  if (f <= 0) f = 1; 
  currentFilePlaying = f;
  digitalWrite(pinRelay, HIGH); // Nyalakan relay untuk auto
  lcd.setCursor(0,1); lcd.print("Ampli Warming Up");
  delay(5000); 
  myDFPlayer.volume(volumeAlat);
  delay(200); 
  myDFPlayer.play(f);
  isPlaying = true;
  isPaused = false;
  playStartTime = millis(); 
}

void stopAudio() {
  currentFilePlaying = 0;
  myDFPlayer.stop();
  // PERBAIKAN: Jangan matikan ampli jika manual Ampli sedang ON
  digitalWrite(pinRelay, manualAmpli ? HIGH : LOW); 
  isPlaying = false;
  isPaused = false;
}

void setup() {
  pinMode(pinRelay, OUTPUT); digitalWrite(pinRelay, LOW); 
  pinMode(pinRelayBel, OUTPUT); digitalWrite(pinRelayBel, LOW); 
  
  pinMode(pinTombol, INPUT_PULLUP); 
  Serial.begin(115200);
  mySoftwareSerial.begin(9600);
  Wire.begin(D6, D5); lcd.init(); lcd.backlight();
  if(!LittleFS.begin()) { Serial.println("FS Error"); }
  
  loadConfig();
  WiFi.begin(ssid, password);
  
  lcd.setCursor(0,0); lcd.print("Koneksi WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Init DFPlayer...");
  
  delay(2000); 
  int retry = 0;
  while (!myDFPlayer.begin(mySoftwareSerial) && retry < 3) { delay(1000); retry++; }
  
  delay(2000); 
  totalFiles = myDFPlayer.readFileCounts();
  if (totalFiles <= 0) { delay(1500); totalFiles = myDFPlayer.readFileCounts(); }
  
  fetchJadwal();
  
  // --- ENDPOINT UNTUK TOMBOL MANUAL ---
  server.on("/ampli", []() {
    if(server.hasArg("state")) manualAmpli = (server.arg("state") == "1");
    // Jika manual Ampli ON ATAU audio otomatis sedang Play, relay harus menyala
    digitalWrite(pinRelay, (manualAmpli || isPlaying) ? HIGH : LOW);
    server.send(200, "text/plain", "OK");
  });

  server.on("/bel", []() {
    if(server.hasArg("state")) {
      if(server.arg("state") == "1") {
        manualBel = true;
        lastBelPing = millis(); // Catat waktu terakhir nerima ping
        digitalWrite(pinRelayBel, HIGH);
      } else {
        manualBel = false;
        // Hanya matikan bel jika bel otomatis sedang tidak berbunyi
        if(!belRelayState) digitalWrite(pinRelayBel, LOW);
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/", handleRoot);
  server.on("/save", []() {
    idKota = server.arg("kota"); volumeAlat = server.arg("vol").toInt();
    for(int i=0; i<7; i++) { aktifTarhim[i] = server.hasArg("a"+String(i)); fileSesi[i] = server.arg("f"+String(i)).toInt(); }
    for(int i=1; i<=totalFiles; i++) { String arg = "d" + String(i); if(server.hasArg(arg)) durasiFiles[i] = server.arg(arg).toInt(); }
    for(int i=0; i<15; i++) {
      bAktif[i] = server.hasArg("bA"+String(i)); bWaktu[i] = server.arg("bW"+String(i));
      bKali[i] = server.arg("bK"+String(i)).toInt(); bDurasi[i] = server.arg("bD"+String(i)).toInt();
    }
    saveConfig(); myDFPlayer.volume(volumeAlat);
    server.sendHeader("Location", "/"); server.send(303);
  });
  
  server.on("/playtest", []() { playAudio(server.arg("n").toInt()); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/pause", []() { if(!isPaused){myDFPlayer.pause(); isPaused=true;}else{myDFPlayer.start(); isPaused=false;} server.sendHeader("Location", "/"); server.send(303); });
  server.on("/stop", []() { stopAudio(); server.sendHeader("Location", "/"); server.send(303); });
  
  server.begin();
  lcd.clear();
}

void loop() {
  server.handleClient();
  timeClient.update();
  
  // PROTEKSI FAILSAFE: Jika tidak nerima ping > 1.5 detik, asumsikan HP putus!
  if (manualBel && (millis() - lastBelPing > 1500)) {
    manualBel = false;
    // Matikan relay hanya jika sistem bel otomatis tidak sedang mengambil alih
    if (!belRelayState) digitalWrite(pinRelayBel, LOW);
  }

  if (digitalRead(pinTombol) == LOW) { 
    delay(200); 
    lcdMode++; 
    if(lcdMode > 2) lcdMode = 0; 
    lcd.clear(); 
  }
  
  int h = timeClient.getHours();
  int m = timeClient.getMinutes(); 
  int s = timeClient.getSeconds();
  long skrgDetik = (long)h * 3600 + (long)m * 60 + s;
  String jamSkrgStr = timeClient.getFormattedTime();

  if (h == 21 && m == 0 && s == 0) fetchJadwal();
  
  // LOGIKA TARHIM OTOMATIS
  if (!isPlaying && millis() - lastTriggerTime > 60000) {
    String times[] = {vImsak, vSubuh, vTerbit, vDzuhur, vAshar, vMaghrib, vIsya};
    for(int i=0; i<7; i++) {
      if(aktifTarhim[i] && times[i] != "--:--" && times[i] != "null") {
        int th = times[i].substring(0,2).toInt();
        int tm = times[i].substring(3,5).toInt();
        long offsetDetik = (i == 0) ? 0 : (durasiFiles[fileSesi[i]] + 5);
        long targetDetik = (long)th * 3600 + (long)tm * 60 - offsetDetik;
        if (skrgDetik >= targetDetik && skrgDetik <= targetDetik + 5) {
          lastTriggerTime = millis();
          playAudio(fileSesi[i]);
          break;
        }
      }
    }
  }

  // Jika lagu tamat, matikan relay HANYA JIKA ampli manual sedang posisi OFF
  if (myDFPlayer.available()) {
    uint8_t type = myDFPlayer.readType();
    if (type == DFPlayerPlayFinished && isPlaying) {
      digitalWrite(pinRelay, manualAmpli ? HIGH : LOW); 
      isPlaying = false;
      currentFilePlaying = 0; 
    }
  }

  // LOGIKA BEL ELEKTRIK OTOMATIS
  static int lastBelMinute = -1;
  if (!isBelRinging && m != lastBelMinute) {
    String hm = jamSkrgStr.substring(0,5); 
    for(int i=0; i<15; i++) {
      if(bAktif[i] && bWaktu[i] == hm && bWaktu[i] != "--:--") {
        isBelRinging = true;
        belRingsLeft = bKali[i];
        currentBelDuration = bDurasi[i] * 1000;
        belRelayState = true;
        digitalWrite(pinRelayBel, HIGH); 
        lastBelToggleTime = millis();
        break; 
      }
    }
    lastBelMinute = m; 
  }

  if (isBelRinging) {
    if (belRelayState) {
      if (millis() - lastBelToggleTime >= currentBelDuration) {
        belRelayState = false;
        lastBelToggleTime = millis();
        belRingsLeft--;
        // Saat jeda, matikan relay (Kecuali tombol HP sedang ditekan paksa)
        if (!manualBel) digitalWrite(pinRelayBel, LOW); 
      }
    } else {
      if (millis() - lastBelToggleTime >= 2000) {
        if (belRingsLeft > 0) {
          belRelayState = true;
          lastBelToggleTime = millis();
          digitalWrite(pinRelayBel, HIGH); 
        } else {
          isBelRinging = false; 
        }
      }
    }
  }

  // ============================================
  // TAMPILAN LCD DENGAN 3 MODE
  // ============================================
  if (lcdMode == 2) {
    lcd.setCursor(0,0);
    lcd.print("Akses Browser:  ");
    lcd.setCursor(0,1); lcd.print(WiFi.localIP().toString() + "    ");
    
  } else if (lcdMode == 1) {
    long minDiffBel = 999999;
    String nextBelTime = "--:--";
    
    for(int i=0; i<15; i++) {
      if(!bAktif[i] || bWaktu[i] == "--:--") continue;
      int th = bWaktu[i].substring(0,2).toInt();
      int tm = bWaktu[i].substring(3,5).toInt();
      long belDetik = (long)th * 3600 + (long)tm * 60;
      
      if (belDetik > skrgDetik) {
        long diff = belDetik - skrgDetik;
        if (diff < minDiffBel) {
          minDiffBel = diff;
          nextBelTime = bWaktu[i];
        }
      }
    }
    
    if (nextBelTime == "--:--") {
      for(int i=0; i<15; i++) {
        if(bAktif[i] && bWaktu[i] != "--:--") {
          int th = bWaktu[i].substring(0,2).toInt();
          int tm = bWaktu[i].substring(3,5).toInt();
          long belDetikBesok = (long)th * 3600 + (long)tm * 60 + 86400;
          long diff = belDetikBesok - skrgDetik;
          if (diff < minDiffBel) {
            minDiffBel = diff;
            nextBelTime = bWaktu[i];
          }
        }
      }
    }

    lcd.setCursor(0,0);
    lcd.print(jamSkrgStr.substring(0,5));
    lcd.print(" | Bel   ");

    lcd.setCursor(0,1);
    if (isBelRinging || manualBel) { // LCD akan bilang bunyi saat ditekan manual
      lcd.print(" BEL BERBUNYI.. ");
    } else {
      if (nextBelTime != "--:--") {
        char buf[25]; 
        sprintf(buf, "%02ld:%02ld:%02ld| %s ", minDiffBel/3600, (minDiffBel%3600)/60, minDiffBel%60, nextBelTime.c_str());
        lcd.print(buf);
      } else {
        lcd.print("Semua Bel Off   ");
      }
    }

  } else {
    String times[] = {vImsak, vSubuh, vTerbit, vDzuhur, vAshar, vMaghrib, vIsya};
    String lbl[] = {"Imsak", "Subuh", "Terbit", "Dzuhur", "Ashar", "Maghrb", "Isya"};
    long minDiffAdzan = 999999; 
    long targetCountdown = 0;
    String nextTime = "--:--";
    String nextLbl = "Jadwal";

    for(int i=0; i<7; i++) {
      if(times[i] == "--:--" || times[i] == "null" || !aktifTarhim[i]) continue;
      int th = times[i].substring(0,2).toInt();
      int tm = times[i].substring(3,5).toInt();
      long adzanDetik = (long)th * 3600 + (long)tm * 60;
      
      if (adzanDetik <= skrgDetik) continue;

      long diffToAdzan = adzanDetik - skrgDetik;
      if (diffToAdzan < minDiffAdzan) {
         minDiffAdzan = diffToAdzan;
         nextTime = times[i]; 
         nextLbl = lbl[i];
         long offsetDetik = (i == 0) ? 0 : (durasiFiles[fileSesi[i]] + 5);
         long startDetik = adzanDetik - offsetDetik;
         targetCountdown = startDetik - skrgDetik;
         if (targetCountdown < 0) targetCountdown = 0; 
      }
    }
    
    if (nextTime == "--:--") {
      for(int i=0; i<7; i++) {
        if(times[i] != "--:--" && times[i] != "null" && aktifTarhim[i]) {
          int th = times[i].substring(0,2).toInt();
          int tm = times[i].substring(3,5).toInt();
          long adzanDetik = (long)th * 3600 + (long)tm * 60;
          long offsetDetik = (i == 0) ? 0 : (durasiFiles[fileSesi[i]] + 5);
          long startDetikBesok = adzanDetik + 86400 - offsetDetik; 
          targetCountdown = startDetikBesok - skrgDetik;
          nextTime = times[i];
          nextLbl = lbl[i];
          break; 
        }
      }
    }

    lcd.setCursor(0,0);
    lcd.print(jamSkrgStr.substring(0,5)); // Hapus detik (Opsi ke-2 yang lebih rapi)
    lcd.print(" | ");
    lcd.print(nextLbl);
    for(int i=0; i<(7 - nextLbl.length()); i++) lcd.print(" "); 

    lcd.setCursor(0,1);
    // Info layar utama juga menginformasikan jika bel/ampli sedang hidup
    if (isBelRinging || manualBel) {
      lcd.print(" BEL BERBUNYI.. ");
    } else if (isPlaying) {
      lcd.print("   PLAYING...   ");
    } else {
      if (nextTime != "--:--") {
        char buf[25]; 
        sprintf(buf, "%02ld:%02ld:%02ld| %s ", targetCountdown/3600, (targetCountdown%3600)/60, targetCountdown%60, nextTime.c_str());
        lcd.print(buf);
      } else {
        lcd.print("Semua Tarhim Off");
      }
    }
  }
}
/*
 * ======================================================================================
 * AKILLI İNVERTER (VOLTRONIC PI30 / WATCHPOWER) & DALY 16S LiFePO4 BMS KÖPRÜSÜ (ESP32)
 * ======================================================================================
 * Sistem Özellikleri:
 *  - Akü Paketi     : 48V 280Ah (16S LiFePO4 - 14.3 kWh) + Daly 8-24S 1A Aktif Balans BMS
 *  - Daly BMS Hattı : %100 Kablolu RS485 (HW-519-R4 Otomatik Yön -> Serial1 GPIO16/GPIO17 @ 9600)
 *  - İnverter Hattı : Voltronic PI30 RS232 (MAX3232 DB9 -> Serial2 RX=GPIO19, TX=GPIO18 @ 2400)
 *  - Otomasyon      : 10-15 Yıl Pil Ömrü Profili (%20-%92, 80A->40A->10A) + 3 Kademeli Termal Bypass
 *  - Uzaktan Erişim : Çift Çekirdek Yerel Web Arayüzü + Gerçek Zamanlı MQTT + Android APK + Wi-Fi OTA
 *  - EEPROM Koruması: İnvertere yalnızca kademe değiştiğinde komut gönderir (Min 45sn aralık)
 * ======================================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "energy_stats.h"

// ======================= 1. WI-FI VE AĞ AYARLARI =======================
// Kişisel Wi-Fi ve MQTT bilgilerinizi "secrets.h" dosyasına yazın (örnek: secrets.example.h)
const char* WIFI_SSID     = WIFI_SSID_CFG;
const char* WIFI_PASS     = WIFI_PASS_CFG;

// Daly BMS Kablolu RS485 (HW-519-R4 -> Serial1) Bilgileri
char        BMS_IP[24]    = "RS485 (HW-519)";
uint8_t     bmsHostAddr   = 0x40; // Otomatik 0x40 <-> 0x80 algılar
uint8_t     bmsFailCount  = 0;
int         bmsRxPin      = 16;   // Açılışta 3.3V çıkış pinini otomatik 16/17 olarak algılar
int         bmsTxPin      = 17;
int         bmsBaud       = 9600;
String      bmsDiagInfo   = "RS485 Baslatiliyor...";

// Gerçek Zamanlı MQTT (Android APK / Mobil Veri 4.5G-5G & Her Yerden Erişim)
const char*    MQTT_BROKER   = "broker.emqx.io";
const uint16_t MQTT_PORT     = 1883;
const char*    MQTT_T_STATUS = MQTT_PREFIX_CFG "/status";
const char*    MQTT_T_STATS  = MQTT_PREFIX_CFG "/stats";
const char*    MQTT_T_CMD    = MQTT_PREFIX_CFG "/cmd";
const char*    MQTT_T_CFG    = MQTT_PREFIX_CFG "/cfg";

WiFiClient   mqttNetClient;
PubSubClient mqttClient(mqttNetClient);
uint32_t     lastMqttTryMs   = 0;
uint32_t     lastMqttPubMs   = 0;

// ======================= 2. DONANIM PİNLERİ (ESP32) ====================
// MAX3232 DB9 RS232 -> ESP32 Serial2 (İnverter WatchPower Portu - DOGRULANDI!)
int         invRxPin      = 19;   // MAX3232 TXD -> GPIO 19 (3.3V Aktif)
int         invTxPin      = 18;   // MAX3232 RXD -> GPIO 18
int         invBaud       = 2400; // Standart Voltronic PI30 / WatchPower RS232 hızı
String      invDiagInfo   = "PI30 BAGLI (RX=19, TX=18)";

// HW-519-R4 RS485 -> ESP32 Serial1 (Daly BMS Kablolu RS485 Portu: GPIO 16 & GPIO 17)
SemaphoreHandle_t bmsMutex = nullptr;

// ======================= 3. VERİ YAPILARI ==============================
struct DalyBMSData {
  bool     connected       = false;
  float    totalVoltage    = 0.0;   // Volt (örn: 50.2V)
  float    current         = 0.0;   // Amper (+ Şarj, - Deşarj)
  float    soc             = 0.0;   // % Doluluk (örn: 38.1%)
  float    remainAh        = 0.0;   // Kalan Ah
  float    maxCellV        = 0.0;   // En yüksek hücre voltajı (V)
  uint8_t  maxCellNum      = 0;     // En yüksek hücre numarası (1-16)
  float    minCellV        = 0.0;   // En düşük hücre voltajı (V)
  uint8_t  minCellNum      = 0;     // En düşük hücre numarası (1-16)
  float    cellDiffMv      = 0.0;   // Hücreler arası fark (mV)
  int      tempMax         = 0;     // Maks sıcaklık (°C)
  int      tempMin         = 0;     // Min sıcaklık (°C)
  bool     chgMosfet       = false;
  bool     disMosfet       = false;
  uint8_t  stateCode       = 0;     // 0=Boşta, 1=Şarjda, 2=Deşarjda
  uint8_t  cellCount       = 16;
  uint8_t  ntcCount        = 1;
  uint16_t cycles          = 0;     // Döngü Sayısı (0x94)
  uint16_t balanceMask     = 0;     // 16 hücre aktif balans bit maskesi (0x97)
  String   faultText       = "Arıza Yok"; // Aktif BMS Arıza/Alarm (0x98)
  float    cellVoltages[16] = {0};
  uint32_t lastUpdateMs    = 0;
} bms;

struct DalyBmsSettings {
  bool     valid      = false;
  float    ratedAh    = 280.0;  // 0x50: Nominal Kapasite (Ah)
  float    nomCellV   = 3.200;  // 0x50: Nominal Hücre Voltajı (V)
  uint16_t sleepSec   = 3600;   // 0x53: Uyku Zamanı (sn)
  float    cellOvp1   = 3.550;  // 0x59: Hücre Yüksek Uyarı (V)
  float    cellOvp2   = 3.600;  // 0x59: Hücre Yüksek Kesme (V)
  float    cellUvp1   = 2.900;  // 0x59: Hücre Düşük Uyarı (V)
  float    cellUvp2   = 2.850;  // 0x59: Hücre Düşük Kesme (V)
  float    packOvp2   = 57.6;   // 0x5A: Paket Yüksek Kesme (V)
  float    packUvp2   = 45.6;   // 0x5A: Paket Düşük Kesme (V)
  int      chgOcp1    = 160;    // 0x5B: Şarj Akım Uyarı (A)
  int      chgOcp2    = 200;    // 0x5B: Şarj Akım Kesme (A)
  int      disOcp1    = 160;    // 0x5B: Deşarj Akım Uyarı (A)
  int      disOcp2    = 200;    // 0x5B: Deşarj Akım Kesme (A)
  int      chgTempHi  = 55;     // 0x5C: Şarj Maks Sıcaklık (°C)
  int      chgTempLo  = 0;      // 0x5C: Şarj Min Sıcaklık (°C)
  int      disTempHi  = 55;     // 0x5C: Deşarj Maks Sıcaklık (°C)
  int      disTempLo  = 0;      // 0x5C: Deşarj Min Sıcaklık (°C)
  float    balStartV  = 3.200;  // 0x5F: 1A Balans Başlama Voltajı (V)
  int      balDiffMv  = 20;     // 0x5F: 1A Balans Başlama Farkı (mV)
} bmsSet;
uint32_t lastBmsSetPollMs = 0;

struct InverterData {
  bool     connected       = false;
  float    gridVoltage     = 0.0;   // Şebeke Voltajı (V)
  float    gridFreq        = 0.0;   // Şebeke Frekansı (Hz)
  float    acOutVoltage    = 0.0;   // AC Çıkış Voltajı (V)
  float    acOutFreq       = 0.0;   // AC Çıkış Frekansı (Hz)
  int      acLoadVA        = 0;     // Görünür Güç (VA)
  int      acLoadWatt      = 0;     // Aktif Ev Yükü (Watt)
  int      loadPercent     = 0;     // Yük Yüzdesi (%)
  float    invBattVoltage  = 0.0;   // İnverterin ölçtüğü Akü Voltajı (V)
  int      chgCurrent      = 0;     // İnverter Şarj Akımı (A)
  int      disCurrent      = 0;     // İnverter Deşarj Akımı (A)
  int      invTemp         = 0;     // İnverter Sıcaklığı (°C)
  float    pvCurrent       = 0.0;   // Solar Panel Akımı (A)
  float    pvVoltage       = 0.0;   // Solar Panel Voltajı (V)
  int      pvPowerWatt     = 0;     // Solar Üretim Gücü (W)
  String   workMode        = "Bilinmiyor"; // SBU / Line / Battery
  String   lastCmdSent     = "Yok";
  uint32_t lastUpdateMs    = 0;
} inv;

struct InverterSettings {
  bool   valid      = false;
  int    ratedW     = 5500;
  int    outPrio    = 2;     // Prog 01: 0=UTI, 1=SOL, 2=SBU
  int    maxChgA    = 80;    // Prog 02: Maks Toplam Şarj Akımı (A)
  int    acRange    = 1;     // Prog 03: 0=APL, 1=UPS
  int    battType   = 2;     // Prog 05: 0=AGM, 1=FLD, 2=USE
  int    gridChgA   = 10;    // Prog 11: Şebeke Şarj Akımı (A)
  float  backGridV  = 46.0;  // Prog 12: Şebekeye Dönüş (PBCV)
  float  backBatV   = 53.0;  // Prog 13: Aküye Dönüş (PBDV)
  int    chgPrio    = 3;     // Prog 16: 0=UTI, 1=CSO, 2=SNU, 3=OSO
  float  bulkV      = 55.2;  // Prog 26: Bulk Voltajı (PCVV)
  float  floatV     = 53.6;  // Prog 27: Float Voltajı (PBFT)
  float  cutOffV    = 45.0;  // Prog 29: Düşük Kesme (PSDV)
  bool   flagBuz    = true;  // Buzzer (a)
  bool   flagByPass = true;  // Aşırı Yük Bypass (b)
  bool   flagPwrSav = false; // Güç Tasarrufu (j)
  bool   flagLcd    = true;  // Ekran Işığı (x)
  String qflagRaw   = "";
} invSet;

struct SmartConfig {
  bool     autoManage      = true;  // Otomatik BMS yönetimi aktif mi?
  float    socToGrid       = 20.0;  // %20 altına inince Şebekeye (UTI) geç (Pil Ömrü Profili)
  float    socToBattery    = 30.0;  // %30 üstüne çıkınca Aküye/Solara (SBU) dön
  float    maxSocLimit     = 90.0;  // ÜST ŞARJ SINIRI (%): %90 dolunca 52.8V/0A durdur (%100 = Sınırsız)
  float    socGridChgOn    = 12.0;  // %12 altına düşerse acil şebeke şarjını (10A) aç
  float    socGridChgOff   = 20.0;  // %20 olunca şebeke şarjını kapat (Sadece Güneş OSO)
  float    cellLowCutV     = 3.00;  // Herhangi bir hücre 3.00V'a inerse Şebekeye geç (uzun ömür)
  float    cellTaper1V     = 3.45;  // Hücre >= 3.45V -> Voltajı 55.2/54.0V'a + Akımı 40A'e düşür
  float    cellTaper2V     = 3.49;  // Hücre >= 3.49V -> 1A Balans Hızı (10A)
  float    cellEmergV      = 3.55;  // Acil tepe freni (BMS 3.60V kesmesine asla girmesin)
  int      maxChargeAmp    = 100;   // 10x550W + 280Ah LiFePO4 Hızlı Boost Şarj Hızı (100A ~ 0.35C)
  int      midChargeAmp    = 40;    // Kademe-1 Yumuşatma / Termal Kısma-1 Hızı (40A ~ 2.1kW)
  int      balChargeAmp    = 10;    // Kademe-2 Balans / Termal Kısma-2 Hızı (10A ~ 540W)
  int      gridChargeAmp   = 10;    // Şebekeden acil şarj akımı (10A)
  float    bulkVoltage     = 56.8;  // Hızlı Boost Bulk Voltajı (3.55V/hücre - kablo düşümünü yener!)
  float    floatVoltage    = 56.0;  // Hızlı Boost Float Voltajı (3.50V/hücre - Voltronic erken Float hatasını yok eder!)
  int      invTempWarn     = 65;    // İnverter Termal Kademe-1 (°C): Şarjı 40A'e kıs (PI30 araştırma onaylı)
  int      invTempHigh     = 72;    // İnverter Termal Kademe-2 (°C): Şarjı 10A'e kıs
  int      invTempBypass   = 78;    // İnverter Termal Kademe-3 (°C): ACİL ŞEBEKE BYPASS (POP00 + 10A)
} cfg;

// Durum takip değişkenleri (EEPROM Koruması için)
int      currentTargetChargeAmp = -1;
String   currentTargetOutputPop = "";
String   currentTargetChgPcp    = "";
bool     baseLiFePO4Synced      = false;
bool     socCapActive           = false; // Üst % SoC şarj sınırı şu an devrede mi?
int      chargeVoltStage        = -1;    // 0=BOOST(56.8/56.0V), 1=YUMUSAT(55.2/54.0V), 2=DURDUR(52.8V/0A)
int      thermalLevel           = 0;     // 0=Normal, 1=Kısma1(40A), 2=Kısma2(10A), 3=ACİL BYPASS(POP00)
bool     thermalBypassActive    = false; // İnverter aşırı ısınma acil şebeke bypass aktif mi?
bool     forceSmartCheck        = true;  // Açılışta veya ayar değişince 45sn beklemeden hemen uygula
String   invQpiriRaw            = "Okunmadı";
String   smartStatusMsg         = "Hazır";
uint32_t lastInverterCmdMs      = 0;
uint32_t lastBmsPollMs          = 0;
uint32_t lastInvPollMs          = 0;
uint32_t lastQpiriPollMs        = 0;
uint32_t lastCloudSyncMs        = 0;

WebServer server(80);
Preferences prefs;
SemaphoreHandle_t invMutex  = nullptr;
SemaphoreHandle_t jsonMutex = nullptr;
String cachedStatusJson     = "{}";

// ======================= 4. VOLTRONIC CRC16 HESAPLAMA ==================
// WatchPower / Voltronic komutları (QPIGS, POP00, MNCHGC060 vb.) özel CRC16 ister
uint16_t calcVoltronicCRC(const uint8_t *pin, uint8_t len) {
  uint16_t crc = 0;
  const uint16_t crc_ta[16] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef
  };
  while (len-- != 0) {
    uint8_t da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*pin >> 4)];
    da = ((uint8_t)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*pin & 0x0f)];
    pin++;
  }
  uint8_t bCRCLow = crc;
  uint8_t bCRCHign = (uint8_t)(crc >> 8);
  if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a) bCRCLow++;
  if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a) bCRCHign++;
  crc = ((uint16_t)bCRCHign) << 8;
  crc += bCRCLow;
  return crc;
}

// İnvertere WatchPower komutu gönder ve yanıtı oku (Thread-Safe)
String sendInverterCommand(const String& cmd, uint32_t timeoutMs = 1200) {
  if (invMutex) xSemaphoreTake(invMutex, pdMS_TO_TICKS(3000));
  while (Serial2.available()) Serial2.read(); // Tamponu temizle

  uint16_t crc = calcVoltronicCRC((const uint8_t*)cmd.c_str(), cmd.length());
  Serial2.print(cmd);
  Serial2.write((uint8_t)(crc >> 8));
  Serial2.write((uint8_t)(crc & 0xFF));
  Serial2.write('\r');

  uint8_t raw[256];
  int rawLen = 0;
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    while (Serial2.available()) {
      uint8_t b = (uint8_t)Serial2.read();
      if (b == '\r') {
        if (invMutex) xSemaphoreGive(invMutex);
        // Son 2 raw byte her zaman Voltronic CRC16'dır — tam olarak 2 byte'ı at!
        int cleanLen = (rawLen >= 3 && raw[0] == '(') ? (rawLen - 2) : rawLen;
        String resp = "";
        for (int i = 0; i < cleanLen; i++) {
          if (raw[i] >= 32 && raw[i] <= 126 && raw[i] != '"' && raw[i] != '\\') {
            resp += (char)raw[i];
          }
        }
        return resp;
      }
      if (rawLen < 255) raw[rawLen++] = b;
    }
    delay(5);
  }
  if (invMutex) xSemaphoreGive(invMutex);
  String resp = "";
  for (int i = 0; i < rawLen; i++) {
    if (raw[i] >= 32 && raw[i] <= 126 && raw[i] != '"' && raw[i] != '\\') {
      resp += (char)raw[i];
    }
  }
  return resp;
}

// ======================= 5. İNVERTER VERİ OKUMA (QPIGS) ================
// MAX3232'nin hangi pine takılı olduğunu ve TX/RX yönünü otomatik bulur
String scanInverterPins() {
  String report = "";
  const int testPins[] = {16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 13, 14, 4, 5};
  Serial2.end();
  delay(20);
  report += "Aktif(3.3V) Pinler: ";
  for (int p : testPins) {
    pinMode(p, INPUT_PULLDOWN);
    delay(3);
    if (digitalRead(p) == HIGH) {
      report += "GPIO" + String(p) + " ";
    }
    pinMode(p, INPUT);
  }

  // Olası RX/TX çiftlerini sırayla dene (Normal 16/17, Ters 17/16, 18/19, 19/18)
  const int pairs[][2] = {{16,17}, {17,16}, {18,19}, {19,18}, {16,19}, {17,18}};
  for (int i = 0; i < 6; i++) {
    int rxP = pairs[i][0];
    int txP = pairs[i][1];
    Serial2.begin(2400, SERIAL_8N1, rxP, txP);
    delay(40);
    String r = sendInverterCommand("QPIGS", 900);
    if (r.length() == 0) r = sendInverterCommand("QPI", 600);
    if (r.startsWith("(")) {
      invRxPin = rxP;
      invTxPin = txP;
      invDiagInfo = "BULUNDU! RX=GPIO" + String(rxP) + " TX=GPIO" + String(txP) + " Yanit:" + r.substring(0, 15);
      return report + " | " + invDiagInfo;
    }
    Serial2.end();
  }

  // Bulunamadıysa mevcut veya ters 16/17'de bırak
  invRxPin = (invRxPin == 16) ? 17 : 16;
  invTxPin = (invRxPin == 16) ? 17 : 16;
  Serial2.begin(2400, SERIAL_8N1, invRxPin, invTxPin);
  invDiagInfo = report + " | Denenen: RX=" + String(invRxPin) + " TX=" + String(invTxPin);
  return invDiagInfo;
}

// İnverterin TÜM ayarlarını (QPIRI + QFLAG) canlı okur ve invSet yapısına yazar
void pollInverterSettings() {
  String qpiri = sendInverterCommand("QPIRI", 900);
  if (qpiri.startsWith("(") && qpiri.length() > 40) {
    invQpiriRaw = qpiri;
    float p[25] = {0};
    int idx = 0, st = 1;
    for (int i = 1; i < (int)qpiri.length() && idx < 25; i++) {
      if (qpiri[i] == ' ' || i == (int)qpiri.length() - 1) {
        int endIdx = (qpiri[i] == ' ') ? i : i + 1;
        p[idx++] = qpiri.substring(st, endIdx).toFloat();
        st = i + 1;
      }
    }
    invSet.valid     = true;
    invSet.ratedW    = (int)p[6];
    invSet.backGridV = p[8];
    invSet.cutOffV   = p[9];
    invSet.bulkV     = p[10];
    invSet.floatV    = p[11];
    invSet.battType  = (int)p[12];
    invSet.gridChgA  = (int)p[13];
    invSet.maxChgA   = (int)p[14];
    invSet.acRange   = (int)p[15];
    invSet.outPrio   = (int)p[16];
    invSet.chgPrio   = (int)p[17];
    invSet.backBatV  = p[22];
  }
  String qf = sendInverterCommand("QFLAG", 500);
  if (qf.startsWith("(") && qf.length() >= 4) {
    invSet.qflagRaw = qf;
    int ePos = qf.indexOf('E');
    int dPos = qf.indexOf('D');
    if (ePos >= 0 && dPos > ePos) {
      String en = qf.substring(ePos + 1, dPos);
      invSet.flagBuz    = (en.indexOf('a') >= 0);
      invSet.flagByPass = (en.indexOf('b') >= 0);
      invSet.flagPwrSav = (en.indexOf('j') >= 0);
      invSet.flagLcd    = (en.indexOf('x') >= 0);
    }
  }
}

void pollInverter() {
  String r = sendInverterCommand("QPIGS", 700);
  // Örnek QPIGS yanıtı:
  // (228.0 50.0 230.0 50.0 0450 0390 009 380 50.20 010 045 0038 012.5 215.0 50.15 00000 00010110 00 00 02680 010
  if (r.startsWith("(") && r.length() > 40) {
    invDiagInfo = "BAGLI (RX=GPIO" + String(invRxPin) + ", TX=GPIO" + String(invTxPin) + ")";
    String data = r.substring(1);
    float v[21] = {0};
    int idx = 0;
    int start = 0;
    for (int i = 0; i < (int)data.length() && idx < 21; i++) {
      if (data[i] == ' ') {
        v[idx++] = data.substring(start, i).toFloat();
        start = i + 1;
      }
    }
    inv.connected      = true;
    inv.gridVoltage    = v[0];
    inv.gridFreq       = v[1];
    inv.acOutVoltage   = v[2];
    inv.acOutFreq      = v[3];
    inv.acLoadVA       = (int)v[4];
    inv.acLoadWatt     = (int)v[5];
    inv.loadPercent    = (int)v[6];
    inv.invBattVoltage = v[8];
    inv.chgCurrent     = (int)v[9];
    inv.invTemp        = (int)v[11];
    inv.pvCurrent      = v[12];
    inv.pvVoltage      = v[13];
    inv.disCurrent     = (int)v[15];
    inv.pvPowerWatt    = (int)v[19];
    if (inv.pvPowerWatt == 0 && inv.pvVoltage > 10) {
      inv.pvPowerWatt = (int)(inv.pvVoltage * inv.pvCurrent);
    }
    inv.lastUpdateMs   = millis();

    // İnverterin gerçek anlık çalışma modunu (QMOD) oku
    String m = sendInverterCommand("QMOD", 400);
    if (m.startsWith("(") && m.length() >= 2) {
      char mc = m[1];
      if      (mc == 'B') inv.workMode = "Akü / Solar Modu (B)";
      else if (mc == 'L') inv.workMode = "Şebeke Modu (L)";
      else if (mc == 'S') inv.workMode = "Bekleme (S)";
      else if (mc == 'F') inv.workMode = "Hata (F)";
      else if (mc == 'P') inv.workMode = "Açılıyor (P)";
    }
  } else {
    if (millis() - inv.lastUpdateMs > 15000) inv.connected = false;
  }
}

// ======================= 6. DALY BMS KABLOLU RS485 (HW-519-R4) OKUMA ===
void initBmsRs485AutoPins() {
  // HW-519-R4'ün 3.3V çıkış (RXD/RO) pininin 16'da mı 17'de mi olduğunu otomatik algıla
  pinMode(16, INPUT_PULLDOWN);
  pinMode(17, INPUT_PULLDOWN);
  delay(8);
  int p16 = digitalRead(16);
  int p17 = digitalRead(17);
  pinMode(16, INPUT);
  pinMode(17, INPUT);
  if (p17 == HIGH && p16 == LOW) {
    bmsRxPin = 17;
    bmsTxPin = 16;
  } else {
    bmsRxPin = 16;
    bmsTxPin = 17;
  }
  Serial1.begin(bmsBaud, SERIAL_8N1, bmsRxPin, bmsTxPin);
  bmsDiagInfo = "RS485 (RX=" + String(bmsRxPin) + ",TX=" + String(bmsTxPin) + ")";
}

// RS485 üzerinden tek 13-byte Daly komutu gönderip yanıtı oku (otomatik yön kontrollü HW-519-R4)
bool sendDalyRs485(uint8_t hostAddr, uint8_t cmdId, uint8_t* respBuf, int expectedFrames = 1) {
  if (bmsMutex) xSemaphoreTake(bmsMutex, pdMS_TO_TICKS(2000));
  uint8_t pkt[13] = {0xA5, hostAddr, cmdId, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t sum = 0;
  for (int i = 0; i < 12; i++) sum += pkt[i];
  pkt[12] = sum;

  while (Serial1.available()) Serial1.read();
  Serial1.write(pkt, 13);
  Serial1.flush(); // HW-519-R4 otomatik yön değişimi için tüm byte'ların çıkışını bekle

  int totalExpected = expectedFrames * 13;
  int got = 0;
  uint32_t t0 = millis();
  uint32_t maxWait = (expectedFrames > 1) ? 900 : 350;
  // İlk byte 0xA5 gelene kadar olası yankı/gürültüyü atla
  bool synced = false;
  while (millis() - t0 < maxWait && got < totalExpected) {
    while (Serial1.available() && got < totalExpected) {
      uint8_t b = (uint8_t)Serial1.read();
      if (!synced) {
        if (b == 0xA5) {
          synced = true;
          respBuf[got++] = b;
        }
      } else {
        respBuf[got++] = b;
      }
    }
    delay(3);
  }
  if (bmsMutex) xSemaphoreGive(bmsMutex);

  // Eğer RS485 hattı kendi gönderdiğimiz paketi yankıladıysa (respBuf[1] == hostAddr), bir sonraki 0xA5 0x01 paketine kaydır
  if (got >= 26 && respBuf[0] == 0xA5 && respBuf[1] == hostAddr && respBuf[13] == 0xA5 && respBuf[14] == 0x01) {
    memmove(respBuf, respBuf + 13, got - 13);
    got -= 13;
  }
  return (got >= 13 && respBuf[0] == 0xA5 && respBuf[1] == 0x01 && respBuf[2] == cmdId);
}

void pollDalyBMS() {
  uint8_t buf[13 * 10];

  // 0x90: Toplam Voltaj, Akım, SoC (Önce kayıtlı host adresi 0x40, olmazsa 0x80 dene)
  bool ok90 = sendDalyRs485(bmsHostAddr, 0x90, buf, 1);
  if (!ok90) {
    uint8_t altAddr = (bmsHostAddr == 0x40) ? 0x80 : 0x40;
    if (sendDalyRs485(altAddr, 0x90, buf, 1)) {
      bmsHostAddr = altAddr;
      ok90 = true;
    }
  }

  if (!ok90) {
    bmsFailCount++;
    if (bmsFailCount >= 4) bms.connected = false;
    return;
  }

  bmsFailCount = 0;
  snprintf(BMS_IP, sizeof(BMS_IP), "RS485 0x%02X", bmsHostAddr);
  {
    uint16_t rawV   = (buf[4] << 8) | buf[5];
    uint16_t rawCur = (buf[8] << 8) | buf[9];
    uint16_t rawSoc = (buf[10] << 8) | buf[11];

    bms.totalVoltage = rawV * 0.1f;
    bms.current      = ((int32_t)rawCur - 30000) * 0.1f;
    bms.soc          = rawSoc * 0.1f;
    bms.connected    = true;
    bms.lastUpdateMs = millis();
  }
  delay(25);

  // 0x91: Max ve Min Hücre Voltajı
  if (sendDalyRs485(bmsHostAddr, 0x91, buf, 1)) {
    uint16_t maxMv = (buf[4] << 8) | buf[5];
    bms.maxCellV   = maxMv / 1000.0f;
    bms.maxCellNum = buf[6];
    uint16_t minMv = (buf[7] << 8) | buf[8];
    bms.minCellV   = minMv / 1000.0f;
    bms.minCellNum = buf[9];
    bms.cellDiffMv = (float)(maxMv - minMv);
  }
  delay(25);

  // 0x92: Sıcaklıklar
  if (sendDalyRs485(bmsHostAddr, 0x92, buf, 1)) {
    bms.tempMax = (int)buf[4] - 40;
    bms.tempMin = (int)buf[6] - 40;
  }
  delay(25);

  // 0x93: MOSFET, Durum ve Kalan Ah
  if (sendDalyRs485(bmsHostAddr, 0x93, buf, 1)) {
    bms.stateCode = buf[4];
    bms.chgMosfet = (buf[5] == 1);
    bms.disMosfet = (buf[6] == 1);
    uint32_t remMah = ((uint32_t)buf[8] << 24) | ((uint32_t)buf[9] << 16) | ((uint32_t)buf[10] << 8) | buf[11];
    bms.remainAh = remMah / 1000.0f;
  }
  delay(25);

  // 0x94: Hücre Sayısı, NTC Sayısı ve Döngü (Cycle) Sayısı
  if (sendDalyRs485(bmsHostAddr, 0x94, buf, 1)) {
    if (buf[4] >= 4 && buf[4] <= 24) bms.cellCount = buf[4];
    bms.ntcCount = buf[5];
    bms.cycles   = ((uint16_t)buf[9] << 8) | buf[10];
  }
  delay(25);

  // 0x95: 16 Hücrenin Tek Tek Voltajları (6 çerçeve x 3 hücre)
  if (sendDalyRs485(bmsHostAddr, 0x95, buf, 6)) {
    for (int f = 0; f < 6; f++) {
      uint8_t* fr = &buf[f * 13];
      if (fr[0] == 0xA5 && fr[2] == 0x95) {
        int frameIdx = fr[4] - 1;
        if (frameIdx >= 0 && frameIdx < 6) {
          for (int c = 0; c < 3; c++) {
            int cellIdx = frameIdx * 3 + c;
            if (cellIdx < 16) {
              uint16_t mv = (fr[5 + c * 2] << 8) | fr[6 + c * 2];
              if (mv > 1000 && mv < 5000) {
                bms.cellVoltages[cellIdx] = mv / 1000.0f;
              }
            }
          }
        }
      }
    }
  }
  delay(25);

  // 0x97: 16 Hücre Canlı 1A Aktif Balans Durumu (Bit Maskesi)
  if (sendDalyRs485(bmsHostAddr, 0x97, buf, 1)) {
    bms.balanceMask = (uint16_t)buf[4] | ((uint16_t)buf[5] << 8);
  }
  delay(25);

  // 0x98: Aktif Arıza / Alarm Bayrakları
  if (sendDalyRs485(bmsHostAddr, 0x98, buf, 1)) {
    String f = "";
    if (buf[4] & 0x03) f += "Hücre Yüksek V! ";
    if (buf[4] & 0x0C) f += "Hücre Düşük V! ";
    if (buf[4] & 0x30) f += "Paket Yüksek V! ";
    if (buf[4] & 0xC0) f += "Paket Düşük V! ";
    if (buf[5] & 0x03) f += "Şarj Aşırı Sıcak! ";
    if (buf[5] & 0x0C) f += "Şarj Soğuk! ";
    if (buf[5] & 0x30) f += "Deşarj Sıcak! ";
    if (buf[5] & 0xC0) f += "Deşarj Soğuk! ";
    if (buf[6] & 0x03) f += "Şarj Aşırı Akım! ";
    if (buf[6] & 0x0C) f += "Deşarj Aşırı Akım! ";
    if (buf[6] & 0x30) f += "SoC Yüksek! ";
    if (buf[6] & 0xC0) f += "SoC Düşük! ";
    if (buf[7] & 0x03) f += "Hücre Farkı Yüksek! ";
    if (buf[8] & 0x0F) f += "MOSFET Sıcak/Hata! ";
    bms.faultText = (f.length() > 0) ? f : "Arıza Yok";
  }
}

// Daly BMS'in tüm iç koruma ve 1A balans ayarlarını RS485'ten canlı okur (0x50..0x5F)
void pollDalySettings() {
  if (!bms.connected) return;
  uint8_t buf[32];

  // 0x50: Nominal Kapasite & Hücre Voltajı
  if (sendDalyRs485(bmsHostAddr, 0x50, buf, 1)) {
    uint32_t capMah = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) | ((uint32_t)buf[6] << 8) | buf[7];
    uint16_t nomMv  = ((uint16_t)buf[10] << 8) | buf[11];
    if (capMah >= 10000) bmsSet.ratedAh = capMah / 1000.0f;
    if (nomMv >= 2000 && nomMv <= 4200) bmsSet.nomCellV = nomMv / 1000.0f;
    bmsSet.valid = true;
  }
  delay(30);

  // 0x53: Uyku Zamanlayıcı
  if (sendDalyRs485(bmsHostAddr, 0x53, buf, 1)) {
    bmsSet.sleepSec = ((uint16_t)buf[9] << 8) | buf[10];
  }
  delay(30);

  // 0x59: Hücre OVP/UVP
  if (sendDalyRs485(bmsHostAddr, 0x59, buf, 1)) {
    bmsSet.cellOvp1 = (((uint16_t)buf[4] << 8) | buf[5]) / 1000.0f;
    bmsSet.cellOvp2 = (((uint16_t)buf[6] << 8) | buf[7]) / 1000.0f;
    bmsSet.cellUvp1 = (((uint16_t)buf[8] << 8) | buf[9]) / 1000.0f;
    bmsSet.cellUvp2 = (((uint16_t)buf[10] << 8) | buf[11]) / 1000.0f;
    bmsSet.valid = true;
  }
  delay(30);

  // 0x5A: Paket OVP/UVP
  if (sendDalyRs485(bmsHostAddr, 0x5A, buf, 1)) {
    bmsSet.packOvp2 = (((uint16_t)buf[6] << 8) | buf[7]) * 0.1f;
    bmsSet.packUvp2 = (((uint16_t)buf[10] << 8) | buf[11]) * 0.1f;
  }
  delay(30);

  // 0x5B: Şarj / Deşarj Aşırı Akım Koruma Limitleri
  if (sendDalyRs485(bmsHostAddr, 0x5B, buf, 1)) {
    bmsSet.chgOcp1 = (30000 - (int)(((uint16_t)buf[4] << 8) | buf[5])) / 10;
    bmsSet.chgOcp2 = (30000 - (int)(((uint16_t)buf[6] << 8) | buf[7])) / 10;
    bmsSet.disOcp1 = ((int)(((uint16_t)buf[8] << 8) | buf[9]) - 30000) / 10;
    bmsSet.disOcp2 = ((int)(((uint16_t)buf[10] << 8) | buf[11]) - 30000) / 10;
  }
  delay(30);

  // 0x5C: Sıcaklık Koruma Limitleri
  if (sendDalyRs485(bmsHostAddr, 0x5C, buf, 1)) {
    bmsSet.chgTempHi = (int)buf[5] - 40;
    bmsSet.chgTempLo = (int)buf[7] - 40;
    bmsSet.disTempHi = (int)buf[9] - 40;
    bmsSet.disTempLo = (int)buf[11] - 40;
  }
  delay(30);

  // 0x5F: 1A Aktif Balans Başlama Voltajı & Farkı
  if (sendDalyRs485(bmsHostAddr, 0x5F, buf, 1)) {
    uint16_t bMv = ((uint16_t)buf[4] << 8) | buf[5];
    uint16_t dMv = ((uint16_t)buf[6] << 8) | buf[7];
    if (bMv >= 2500 && bMv <= 4200) bmsSet.balStartV = bMv / 1000.0f;
    if (dMv <= 500) bmsSet.balDiffMv = dMv;
  }
}

// Daly BMS RS485 Yazma Komutu (MOSFET Aç/Kapat, 1A Balans Voltaj/Fark, Hücre OVP/UVP)
bool sendDalyWrite(uint8_t cmdId, const uint8_t data8[8]) {
  if (bmsMutex) xSemaphoreTake(bmsMutex, pdMS_TO_TICKS(2000));
  uint8_t pkt[13] = {0xA5, bmsHostAddr, cmdId, 0x08,
                     data8[0], data8[1], data8[2], data8[3],
                     data8[4], data8[5], data8[6], data8[7], 0};
  uint8_t sum = 0;
  for (int i = 0; i < 12; i++) sum += pkt[i];
  pkt[12] = sum;
  while (Serial1.available()) Serial1.read();
  Serial1.write(pkt, 13);
  Serial1.flush();
  uint8_t resp[26];
  int got = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < 400 && got < 13) {
    while (Serial1.available() && got < 13) resp[got++] = (uint8_t)Serial1.read();
    delay(3);
  }
  if (bmsMutex) xSemaphoreGive(bmsMutex);
  return (got >= 13 && resp[0] == 0xA5 && resp[2] == cmdId);
}

String handleBmsControlCmd(const String& cmd) {
  uint8_t d[8] = {0};
  if (cmd == "BMS:CHGMOS:1" || cmd == "BMS:CHGMOS:0") {
    d[0] = (cmd.endsWith("1")) ? 1 : 0;
    bool ok = sendDalyWrite(0xDA, d);
    return ok ? "Sarj MOSFET -> OK" : "Sarj MOSFET -> Hata";
  }
  if (cmd == "BMS:DISMOS:1" || cmd == "BMS:DISMOS:0") {
    d[0] = (cmd.endsWith("1")) ? 1 : 0;
    bool ok = sendDalyWrite(0xD9, d);
    return ok ? "Desarj MOSFET -> OK" : "Desarj MOSFET -> Hata";
  }
  if (cmd.startsWith("BMS:BAL:")) {
    // Format: BMS:BAL:3400:15 (mV, mV fark)
    int p1 = cmd.indexOf(':', 8);
    if (p1 > 8) {
      uint16_t startMv = constrain(cmd.substring(8, p1).toInt(), 2800, 3600);
      uint16_t diffMv  = constrain(cmd.substring(p1 + 1).toInt(), 5, 200);
      d[0] = startMv >> 8; d[1] = startMv & 0xFF;
      d[2] = diffMv >> 8;  d[3] = diffMv & 0xFF;
      bool ok = sendDalyWrite(0x1F, d);
      delay(150);
      pollDalySettings();
      return ok ? ("1A Balans " + String(startMv) + "mV/" + String(diffMv) + "mV -> OK") : "Balans Yazma Hatasi";
    }
  }
  if (cmd.startsWith("BMS:OVP:")) {
    // Format: BMS:OVP:3550:3600:2900:2850
    int i1 = cmd.indexOf(':', 8);
    int i2 = cmd.indexOf(':', i1 + 1);
    int i3 = cmd.indexOf(':', i2 + 1);
    if (i1 > 0 && i2 > 0 && i3 > 0) {
      uint16_t o1 = constrain(cmd.substring(8, i1).toInt(), 3300, 3700);
      uint16_t o2 = constrain(cmd.substring(i1 + 1, i2).toInt(), 3400, 3750);
      uint16_t u1 = constrain(cmd.substring(i2 + 1, i3).toInt(), 2500, 3100);
      uint16_t u2 = constrain(cmd.substring(i3 + 1).toInt(), 2400, 3000);
      d[0] = o1 >> 8; d[1] = o1 & 0xFF;
      d[2] = o2 >> 8; d[3] = o2 & 0xFF;
      d[4] = u1 >> 8; d[5] = u1 & 0xFF;
      d[6] = u2 >> 8; d[7] = u2 & 0xFF;
      bool ok = sendDalyWrite(0x19, d);
      delay(150);
      pollDalySettings();
      return ok ? "Hucre OVP/UVP -> OK" : "OVP/UVP Yazma Hatasi";
    }
  }
  return "Bilinmeyen BMS Komutu";
}

// ======================= 7. KOMPLE 16S LiFePO4 BMS -> İNVERTER YÖNETİMİ =
// İlk bağlantıda inverter ayarlarını (QPIRI) okuyup 16S LiFePO4 48V 280Ah profiline oturtur
String applyFullLiFePO4Setup(bool forceWrite = false) {
  String qpiri = sendInverterCommand("QPIRI", 900);
  if (qpiri.startsWith("(") && qpiri.length() > 40) {
    invQpiriRaw = qpiri;
  } else if (!forceWrite) {
    return "QPIRI okunamadi";
  }

  // Doğrulanmış 5.5kW PI30 QPIRI alanları:
  // p[8]:BackToGrid(PBCV), p[9]:LowCutOff(PSDV), p[10]:Bulk(PCVV), p[11]:Float(PBFT),
  // p[12]:BattType(2=USE), p[13]:GridChgA(MUCHGC), p[14]:MaxChgA(MCHGC), p[16]:OutPrio, p[17]:ChgPrio, p[22]:BackToBat(PBDV)
  float p[25] = {0};
  int idx = 0, st = 1;
  for (int i = 1; i < (int)qpiri.length() && idx < 25; i++) {
    if (qpiri[i] == ' ' || i == (int)qpiri.length() - 1) {
      int endIdx = (qpiri[i] == ' ') ? i : i + 1;
      p[idx++] = qpiri.substring(st, endIdx).toFloat();
      st = i + 1;
    }
  }

  String log = "";
  auto setIfNeeded = [&](bool cond, const String& cmd) {
    if (forceWrite || cond) {
      String r = sendInverterCommand(cmd, 800);
      log += cmd + ":" + r + " ";
      delay(300);
    }
  };

  // 1) Program 05: Akü Tipi -> USE (PBT02) - LiFePO4 ayarı olmayan inverterde şarttır!
  setIfNeeded((int)p[12] != 2, "PBT02");
  // 2-3) Program 26 & 27: Bulk & Float Voltajı (Üst sınır zaten doluysa yükseltme; aksi halde doğru sırayla uygula!)
  bool allowVoltSync = forceWrite || !bms.connected || (cfg.maxSocLimit >= 99.5f) || (bms.soc < cfg.maxSocLimit);
  if (allowVoltSync) {
    char bCmd[16], fCmd[16];
    snprintf(bCmd, sizeof(bCmd), "PCVV%.1f", cfg.bulkVoltage);
    snprintf(fCmd, sizeof(fCmd), "PBFT%.1f", cfg.floatVoltage);
    if (cfg.bulkVoltage >= p[10]) {
      setIfNeeded(fabs(p[10] - cfg.bulkVoltage) > 0.15f, String(bCmd));
      setIfNeeded(fabs(p[11] - cfg.floatVoltage) > 0.15f, String(fCmd));
    } else {
      setIfNeeded(fabs(p[11] - cfg.floatVoltage) > 0.15f, String(fCmd));
      setIfNeeded(fabs(p[10] - cfg.bulkVoltage) > 0.15f, String(bCmd));
    }
  }
  // 4) Program 29: Düşük Voltaj Kesme -> 45.0V (2.81V/hücre)
  setIfNeeded(fabs(p[9] - 45.0f) > 0.25f, "PSDV45.0");
  // 5) Program 12: Şebekeye Dönüş Voltajı -> 46.0V (karar ESP32'de!)
  setIfNeeded(fabs(p[8] - 46.0f) > 0.25f, "PBCV46.0");
  // 6) Program 13: Aküye Dönüş Voltajı -> 53.0V
  setIfNeeded(fabs(p[22] - 53.0f) > 0.25f, "PBDV53.0");
  // 7) Program 11: Şebeke Şarj Akımı -> 10A
  setIfNeeded((int)p[13] != cfg.gridChargeAmp, "MUCHGC010");

  // Güncel QPIRI'yi tekrar oku ve kaydet
  delay(250);
  String q2 = sendInverterCommand("QPIRI", 900);
  if (q2.startsWith("(") && q2.length() > 40) invQpiriRaw = q2;

  baseLiFePO4Synced = true;
  if (log.length() == 0) log = "Tum 16S LiFePO4 ayarlari uyumlu";
  inv.lastCmdSent = "[LiFePO4 Ayar] " + log;
  return log;
}

// Voltronic PI30 kuralı: PCVV >= PBFT daima korunmalı! Yükseltirken PCVV önce, düşürürken PBFT önce gönderilir.
String setInverterVoltages(float targetBulk, float targetFloat) {
  if (targetFloat > targetBulk) targetFloat = targetBulk;
  char bCmd[16], fCmd[16];
  snprintf(bCmd, sizeof(bCmd), "PCVV%.1f", targetBulk);
  snprintf(fCmd, sizeof(fCmd), "PBFT%.1f", targetFloat);
  String r1, r2;
  if (targetBulk >= invSet.bulkV) {
    r1 = sendInverterCommand(String(bCmd), 800);
    delay(250);
    r2 = sendInverterCommand(String(fCmd), 800);
  } else {
    r1 = sendInverterCommand(String(fCmd), 800);
    delay(250);
    r2 = sendInverterCommand(String(bCmd), 800);
  }
  return r1 + "/" + r2;
}

// 5.5kW Voltronic PI30 için doğrulanan 10A-110A şarj akımı gönderici (MNCHGC0xxx + MCHGCxxx yedek)
String setInverterChargeAmp(int amp, String* usedCmd = nullptr) {
  int stepAmp = ((amp + 5) / 10) * 10; // Geçerli 10A kademelerine yuvarla (010..110)
  stepAmp = constrain(stepAmp, 10, 110);
  char cmdBuf[16];
  snprintf(cmdBuf, sizeof(cmdBuf), "MNCHGC0%03d", stepAmp);
  String r = sendInverterCommand(String(cmdBuf), 800);
  if (r.indexOf("ACK") < 0) {
    snprintf(cmdBuf, sizeof(cmdBuf), "MCHGC%03d", stepAmp);
    r = sendInverterCommand(String(cmdBuf), 800);
  }
  if (usedCmd) *usedCmd = String(cmdBuf);
  return r;
}

void runSmartBmsControl() {
  if (!cfg.autoManage || !inv.connected) return;

  // --- KATMAN -1 (EN YÜKSEK ÖNCELİK): İNVERTER 3 KADEMELİ TERMAL KORUMA & ACİL BYPASS ---
  // BMS bağlantısından bağımsız çalışır; inverter aşırı ısınırsa bekleme süresini atlayıp ANINDA müdahale eder!
  int nextTherm = thermalLevel;
  if (inv.invTemp >= cfg.invTempBypass) {
    nextTherm = 3;
  } else if (thermalLevel < 3) {
    if (inv.invTemp >= cfg.invTempHigh) nextTherm = 2;
    else if (thermalLevel < 2 && inv.invTemp >= cfg.invTempWarn) nextTherm = 1;
  }
  // Soğuma histerezisi (Bypass -13°C [78->65°C], Kademe2 -6°C [72->66°C], Kademe1 -5°C [65->60°C])
  if (thermalLevel == 3 && inv.invTemp <= (cfg.invTempBypass - 13)) {
    nextTherm = (inv.invTemp >= cfg.invTempHigh) ? 2 : ((inv.invTemp >= cfg.invTempWarn) ? 1 : 0);
  } else if (thermalLevel == 2 && inv.invTemp <= (cfg.invTempHigh - 6)) {
    nextTherm = (inv.invTemp >= cfg.invTempWarn) ? 1 : 0;
  } else if (thermalLevel == 1 && inv.invTemp <= (cfg.invTempWarn - 5)) {
    nextTherm = 0;
  }

  bool thermalEscalated = (nextTherm > thermalLevel);

  // Kademe-3: ACİL TERMAL BYPASS (>= 78°C) -> Derhal Şebekeye (POP00) at + Şarjı 10A'e indir!
  if (nextTherm == 3) {
    thermalLevel = 3;
    smartStatusMsg = "ACIL TERMAL BYPASS (" + String(inv.invTemp) + "C -> Sebeke + " + String(cfg.balChargeAmp) + "A)";
    if (!thermalBypassActive || currentTargetOutputPop != "POP00" || currentTargetChargeAmp != cfg.balChargeAmp) {
      String rPop = sendInverterCommand("POP00", 800);
      delay(250);
      String usedCmd;
      String rAmp = setInverterChargeAmp(cfg.balChargeAmp, &usedCmd);
      thermalBypassActive = true;
      currentTargetOutputPop = "POP00";
      currentTargetChargeAmp = cfg.balChargeAmp;
      pollInverterSettings();
      inv.lastCmdSent = "[ACIL TERMAL BYPASS " + String(inv.invTemp) + "C] POP00:" + rPop + " " + usedCmd + ":" + rAmp;
      lastInverterCmdMs = millis();
    }
    return;
  }

  // Termal Bypass'tan soğuyarak çıkış (<= 65°C)
  if (thermalLevel == 3 && nextTherm < 3) {
    thermalLevel = nextTherm;
    thermalBypassActive = false;
    if (!bms.connected || (bms.soc >= cfg.socToBattery && bms.minCellV >= (cfg.cellLowCutV + 0.15f))) {
      String rPop = sendInverterCommand("POP02", 800);
      currentTargetOutputPop = "POP02";
      pollInverterSettings();
      inv.lastCmdSent = "[Termal Normal " + String(inv.invTemp) + "C] Bypass Bitti -> SBU (POP02:" + rPop + ")";
    } else {
      inv.lastCmdSent = "[Termal Normal " + String(inv.invTemp) + "C] Sogudu (SoC dusuk -> Sebeke)";
    }
    forceSmartCheck = true;
    lastInverterCmdMs = millis();
    return;
  }

  thermalLevel = nextTherm;
  thermalBypassActive = false;

  // BMS bağlı değilse sadece Kademe 1-2 termal şarj kısmasını uygula ve çık
  if (!bms.connected) {
    if (thermalLevel >= 1 && (thermalEscalated || forceSmartCheck || (millis() - lastInverterCmdMs >= 45000))) {
      int tAmp = (thermalLevel == 2) ? cfg.balChargeAmp : cfg.midChargeAmp;
      smartStatusMsg = "TERMAL KISMA-" + String(thermalLevel) + " (" + String(inv.invTemp) + "C -> " + String(tAmp) + "A)";
      if (tAmp != currentTargetChargeAmp) {
        String usedCmd;
        String r = setInverterChargeAmp(tAmp, &usedCmd);
        currentTargetChargeAmp = tAmp;
        inv.lastCmdSent = usedCmd + " [" + smartStatusMsg + "] -> " + r;
        lastInverterCmdMs = millis();
      }
      forceSmartCheck = false;
    }
    return;
  }

  // İlk bağlantıda inverterin temel 16S LiFePO4 ayarlarını (USE, 45.0V, PBCV 46V, PBDV 53V) doğrula
  if (!baseLiFePO4Synced) {
    applyFullLiFePO4Setup(false);
    lastInverterCmdMs = millis();
    return;
  }

  // EEPROM koruması: İki otomatik komut arasında en az 45 saniye bekle (termal yükseliş veya yeni ayarda bekleme!)
  if (!thermalEscalated && !forceSmartCheck && (millis() - lastInverterCmdMs < 45000)) return;
  forceSmartCheck = false;

  // --- KATMAN 0: %90 ÜST SINIR VEYA HÜCRE >= 3.51V EMNİYET KESMESİ (AŞAMA 3: 52.8V / 0A DURDUR!) ---
  bool hitSocCap  = (cfg.maxSocLimit < 99.5f && bms.soc >= cfg.maxSocLimit);
  bool hitCellCap = (cfg.maxSocLimit < 99.5f && bms.maxCellV >= 3.51f);
  if (hitSocCap || hitCellCap) {
    smartStatusMsg = hitCellCap
      ? ("UST SINIR EMNIYET (Hucre " + String(bms.maxCellV, 3) + "V -> 52.8V/0A)")
      : ("UST SINIR AKTIF (%" + String(cfg.maxSocLimit, 0) + " Doldu -> 52.8V / 0A Durduruldu)");
    bool alreadyAtCap = invSet.valid && fabs(invSet.bulkV - 52.8f) <= 0.15f && fabs(invSet.floatV - 52.8f) <= 0.15f && invSet.maxChgA <= 10;
    if (!socCapActive || !alreadyAtCap) {
      if (!alreadyAtCap) {
        String rv = setInverterVoltages(52.8f, 52.8f);
        delay(250);
        String ra = setInverterChargeAmp(10);
        pollInverterSettings();
        inv.lastCmdSent = "[Ust Sinir %" + String(cfg.maxSocLimit, 0) + "] 52.8V/0A -> " + rv + " " + ra;
        lastInverterCmdMs = millis();
      }
      socCapActive = true;
      chargeVoltStage = 2;
      currentTargetChargeAmp = 10;
      return;
    }
  } else if (socCapActive && (cfg.maxSocLimit >= 99.5f || (bms.soc <= (cfg.maxSocLimit - 3.0f) && bms.maxCellV < 3.42f))) {
    // Sınırın %3 altına inince veya sınır %100 yapılınca Boost şarj voltajlarını geri aç!
    float tBulk  = (bms.maxCellV >= cfg.cellTaper1V) ? 55.2f : cfg.bulkVoltage;
    float tFloat = (bms.maxCellV >= cfg.cellTaper1V) ? 54.0f : cfg.floatVoltage;
    String rv = setInverterVoltages(tBulk, tFloat);
    socCapActive = false;
    chargeVoltStage = (bms.maxCellV >= cfg.cellTaper1V) ? 1 : 0;
    pollInverterSettings();
    inv.lastCmdSent = "[Sinir Acildi] Boost " + String(tBulk, 1) + "V/" + String(tFloat, 1) + "V -> " + rv;
    forceSmartCheck = true; // Bir sonraki döngüde tam akımı (100A) hemen geri yükselt
    lastInverterCmdMs = millis();
    return;
  }

  // Üst sınır aktifken şarj akımını veya voltajını tekrar artırma
  if (socCapActive) return;

  // --- KATMAN 1 & 4: 3 AŞAMALI DİNAMİK VOLTAJ (BOOST -> YUMUŞATMA) + AKIM + TERMAL KONTROL ---
  // Aşama 0 (BOOST): Hücre < 3.45V iken 56.8V/56.0V + 100A (kablo düşümünü ve erken Float'ı tamamen yener!)
  // Aşama 1 (YUMUŞATMA): Hücre >= 3.45V (veya %100 balans gününde %93+) olunca 55.2V/54.0V + 40A/10A!
  int desiredVoltStage = (chargeVoltStage == 1) ? 1 : 0;
  int desiredAmp = cfg.maxChargeAmp;
  String reason = "BOOST Hizli Sarj (" + String(cfg.bulkVoltage, 1) + "V/" + String(cfg.floatVoltage, 1) + "V " + String(cfg.maxChargeAmp) + "A)";

  if (bms.tempMax <= 2 || bms.tempMax >= 45) {
    desiredVoltStage = 1;
    desiredAmp = cfg.balChargeAmp;
    reason = "Pil Sicaklik Korumasi (" + String(bms.tempMax) + "C -> 55.2V/" + String(cfg.balChargeAmp) + "A)";
  } else if (bms.maxCellV >= cfg.cellEmergV) {
    desiredVoltStage = 1;
    desiredAmp = cfg.balChargeAmp;
    reason = "Acil Tepe Freni (Hucre >= " + String(cfg.cellEmergV, 2) + "V -> 55.2V/" + String(cfg.balChargeAmp) + "A)";
  } else if (bms.maxCellV >= cfg.cellTaper2V || (cfg.maxSocLimit >= 99.5f && bms.soc >= 96.0f) || (bms.cellDiffMv >= 45.0f && bms.maxCellV >= 3.40f)) {
    desiredVoltStage = 1;
    desiredAmp = cfg.balChargeAmp;
    reason = "1A Balans Modu (" + String(cfg.cellTaper2V, 2) + "V -> 55.2V/" + String(cfg.balChargeAmp) + "A)";
  } else if (bms.maxCellV >= cfg.cellTaper1V || (cfg.maxSocLimit >= 99.5f && bms.soc >= 93.0f)) {
    desiredVoltStage = 1;
    desiredAmp = cfg.midChargeAmp;
    reason = "Yumusatma Modu (Hucre >= " + String(cfg.cellTaper1V, 2) + "V -> 55.2V/" + String(cfg.midChargeAmp) + "A)";
  } else if (bms.maxCellV < (cfg.cellTaper1V - 0.04f)) {
    desiredVoltStage = 0;
    desiredAmp = cfg.maxChargeAmp;
    reason = "BOOST Hizli Sarj (" + String(cfg.bulkVoltage, 1) + "V/" + String(cfg.floatVoltage, 1) + "V -> " + String(cfg.maxChargeAmp) + "A)";
  } else if (currentTargetChargeAmp > 0) {
    desiredAmp = currentTargetChargeAmp; // Histerezis bölgesi - gereksiz değiştirme
  }

  // Dinamik Voltaj Geçişi (BOOST 56.8/56.0V <-> YUMUŞATMA 55.2/54.0V)
  float targetBulkV  = (desiredVoltStage == 0) ? cfg.bulkVoltage  : 55.2f;
  float targetFloatV = (desiredVoltStage == 0) ? cfg.floatVoltage : 54.0f;
  bool voltMismatch = invSet.valid && (fabs(invSet.bulkV - targetBulkV) > 0.15f || fabs(invSet.floatV - targetFloatV) > 0.15f);
  if (desiredVoltStage != chargeVoltStage || voltMismatch) {
    String rv = setInverterVoltages(targetBulkV, targetFloatV);
    chargeVoltStage = desiredVoltStage;
    pollInverterSettings();
    inv.lastCmdSent = (desiredVoltStage == 0 ? "[BOOST Voltaj] " : "[Yumusatma Voltaj] ") +
                      String(targetBulkV, 1) + "V/" + String(targetFloatV, 1) + "V -> " + rv;
    forceSmartCheck = true; // Voltaj oturduktan sonra akımı da hemen eşitle
    lastInverterCmdMs = millis();
    return;
  }

  // İnverter Termal Kademe-1 / Kademe-2 Şarj Akımı Kısma (Pil hedef akımını aşağı çek!)
  if (thermalLevel == 2 && desiredAmp > cfg.balChargeAmp) {
    desiredAmp = cfg.balChargeAmp;
    reason = "TERMAL KISMA-2 (Inv " + String(inv.invTemp) + "C -> " + String(desiredAmp) + "A)";
  } else if (thermalLevel == 1 && desiredAmp > cfg.midChargeAmp) {
    desiredAmp = cfg.midChargeAmp;
    reason = "TERMAL KISMA-1 (Inv " + String(inv.invTemp) + "C -> " + String(desiredAmp) + "A)";
  }

  smartStatusMsg = reason;

  if (desiredAmp != currentTargetChargeAmp || (invSet.valid && invSet.maxChgA != (((desiredAmp + 5) / 10) * 10))) {
    String usedCmd;
    String r = setInverterChargeAmp(desiredAmp, &usedCmd);
    currentTargetChargeAmp = desiredAmp;
    pollInverterSettings();
    inv.lastCmdSent = usedCmd + " [" + reason + "] -> " + r;
    lastInverterCmdMs = millis();
    return;
  }

  // --- KATMAN 2: GERÇEK % SoC VE EN DÜŞÜK HÜCREYE GÖRE ŞEBEKE / AKÜ GEÇİŞİ ---
  String desiredPop = currentTargetOutputPop;
  if (bms.soc <= cfg.socToGrid || (bms.minCellV > 1.5f && bms.minCellV <= cfg.cellLowCutV)) {
    desiredPop = "POP00"; // Şebekeye (Utility) geç
  } else if (bms.soc >= cfg.socToBattery && bms.minCellV >= (cfg.cellLowCutV + 0.15f)) {
    desiredPop = "POP02"; // SBU (Akü + Güneş) moduna geç
  }

  if (desiredPop.length() > 0 && desiredPop != currentTargetOutputPop) {
    String r = sendInverterCommand(desiredPop, 800);
    currentTargetOutputPop = desiredPop;
    inv.lastCmdSent = desiredPop + (desiredPop == "POP00" ? " [SoC Dusuk->Sebeke] " : " [SoC Yeterli->Aku/SBU] ") + r;
    lastInverterCmdMs = millis();
    return;
  }

  // --- KATMAN 3: ACİL ŞEBEKE DESTEK ŞARJI (KIŞ / GECE KORUMASI) ---
  String desiredPcp = currentTargetChgPcp;
  if (bms.soc <= cfg.socGridChgOn || (bms.minCellV > 1.5f && bms.minCellV <= 2.85f)) {
    desiredPcp = "PCP02"; // Güneş + Şebeke (10A destek şarjı aç)
  } else if (bms.soc >= cfg.socGridChgOff) {
    desiredPcp = "PCP03"; // Sadece Güneş (OSO - Şebekeden boşuna fatura yazmasın)
  }

  if (desiredPcp.length() > 0 && desiredPcp != currentTargetChgPcp) {
    String r = sendInverterCommand(desiredPcp, 800);
    currentTargetChgPcp = desiredPcp;
    inv.lastCmdSent = desiredPcp + (desiredPcp == "PCP02" ? " [Acil Sebeke Sarji ACIK] " : " [Sadece Gunes OSO] ") + r;
    lastInverterCmdMs = millis();
  }
}

// ======================= 8. JSON VERİ API'Sİ ===========================
String buildStatusJson() {
  String j = "{";
  j += "\"bms\":{";
  j += "\"ok\":" + String(bms.connected ? "true" : "false") + ",";
  j += "\"ip\":\"" + String(BMS_IP) + "\",";
  j += "\"diag\":\"" + bmsDiagInfo + "\",";
  j += "\"v\":" + String(bms.totalVoltage, 2) + ",";
  j += "\"a\":" + String(bms.current, 1) + ",";
  j += "\"w\":" + String((int)(bms.totalVoltage * bms.current)) + ",";
  j += "\"soc\":" + String(bms.soc, 1) + ",";
  j += "\"ah\":" + String(bms.remainAh, 2) + ",";
  j += "\"maxV\":" + String(bms.maxCellV, 3) + ",";
  j += "\"maxN\":" + String(bms.maxCellNum) + ",";
  j += "\"minV\":" + String(bms.minCellV, 3) + ",";
  j += "\"minN\":" + String(bms.minCellNum) + ",";
  j += "\"diff\":" + String((int)bms.cellDiffMv) + ",";
  j += "\"tMax\":" + String(bms.tempMax) + ",";
  j += "\"chgMos\":" + String(bms.chgMosfet ? "true" : "false") + ",";
  j += "\"disMos\":" + String(bms.disMosfet ? "true" : "false") + ",";
  j += "\"st\":" + String(bms.stateCode) + ",";
  j += "\"cyc\":" + String(bms.cycles) + ",";
  j += "\"balMask\":" + String(bms.balanceMask) + ",";
  j += "\"fault\":\"" + bms.faultText + "\",";
  j += "\"ntc\":" + String(bms.ntcCount) + ",";
  j += "\"cells\":[";
  for (int i = 0; i < 16; i++) {
    j += String(bms.cellVoltages[i], 3);
    if (i < 15) j += ",";
  }
  j += "]},";

  j += "\"bmsSet\":{";
  j += "\"ok\":" + String(bmsSet.valid ? "true" : "false") + ",";
  j += "\"ratedAh\":" + String(bmsSet.ratedAh, 1) + ",";
  j += "\"nomV\":" + String(bmsSet.nomCellV, 3) + ",";
  j += "\"sleep\":" + String(bmsSet.sleepSec) + ",";
  j += "\"ovp1\":" + String(bmsSet.cellOvp1, 3) + ",";
  j += "\"ovp2\":" + String(bmsSet.cellOvp2, 3) + ",";
  j += "\"uvp1\":" + String(bmsSet.cellUvp1, 3) + ",";
  j += "\"uvp2\":" + String(bmsSet.cellUvp2, 3) + ",";
  j += "\"pOvp\":" + String(bmsSet.packOvp2, 1) + ",";
  j += "\"pUvp\":" + String(bmsSet.packUvp2, 1) + ",";
  j += "\"cOcp1\":" + String(bmsSet.chgOcp1) + ",";
  j += "\"cOcp2\":" + String(bmsSet.chgOcp2) + ",";
  j += "\"dOcp1\":" + String(bmsSet.disOcp1) + ",";
  j += "\"dOcp2\":" + String(bmsSet.disOcp2) + ",";
  j += "\"cTHi\":" + String(bmsSet.chgTempHi) + ",";
  j += "\"cTLo\":" + String(bmsSet.chgTempLo) + ",";
  j += "\"dTHi\":" + String(bmsSet.disTempHi) + ",";
  j += "\"dTLo\":" + String(bmsSet.disTempLo) + ",";
  j += "\"balV\":" + String(bmsSet.balStartV, 3) + ",";
  j += "\"balMv\":" + String(bmsSet.balDiffMv) + "},";

  j += "\"inv\":{";
  j += "\"ok\":" + String(inv.connected ? "true" : "false") + ",";
  j += "\"gridV\":" + String(inv.gridVoltage, 1) + ",";
  j += "\"gridHz\":" + String(inv.gridFreq, 1) + ",";
  j += "\"outV\":" + String(inv.acOutVoltage, 1) + ",";
  j += "\"loadW\":" + String(inv.acLoadWatt) + ",";
  j += "\"loadPct\":" + String(inv.loadPercent) + ",";
  j += "\"pvV\":" + String(inv.pvVoltage, 1) + ",";
  j += "\"pvA\":" + String(inv.pvCurrent, 1) + ",";
  j += "\"pvW\":" + String(inv.pvPowerWatt) + ",";
  j += "\"chgA\":" + String(inv.chgCurrent) + ",";
  j += "\"disA\":" + String(inv.disCurrent) + ",";
  j += "\"temp\":" + String(inv.invTemp) + ",";
  j += "\"thermLvl\":" + String(thermalLevel) + ",";
  j += "\"thermByp\":" + String(thermalBypassActive ? "true" : "false") + ",";
  j += "\"voltStg\":" + String(chargeVoltStage) + ",";
  j += "\"mode\":\"" + inv.workMode + "\",";
  j += "\"diag\":\"" + invDiagInfo + "\",";
  j += "\"smart\":\"" + smartStatusMsg + "\",";
  j += "\"qpiri\":\"" + invQpiriRaw + "\",";
  j += "\"lastCmd\":\"" + inv.lastCmdSent + "\"},";

  j += "\"set\":{";
  j += "\"ok\":" + String(invSet.valid ? "true" : "false") + ",";
  j += "\"ratedW\":" + String(invSet.ratedW) + ",";
  j += "\"outPrio\":" + String(invSet.outPrio) + ",";
  j += "\"maxChgA\":" + String(invSet.maxChgA) + ",";
  j += "\"acRange\":" + String(invSet.acRange) + ",";
  j += "\"battType\":" + String(invSet.battType) + ",";
  j += "\"gridChgA\":" + String(invSet.gridChgA) + ",";
  j += "\"backGridV\":" + String(invSet.backGridV, 1) + ",";
  j += "\"backBatV\":" + String(invSet.backBatV, 1) + ",";
  j += "\"chgPrio\":" + String(invSet.chgPrio) + ",";
  j += "\"bulkV\":" + String(invSet.bulkV, 1) + ",";
  j += "\"floatV\":" + String(invSet.floatV, 1) + ",";
  j += "\"cutOffV\":" + String(invSet.cutOffV, 1) + ",";
  j += "\"buz\":" + String(invSet.flagBuz ? "true" : "false") + ",";
  j += "\"bypass\":" + String(invSet.flagByPass ? "true" : "false") + ",";
  j += "\"pwrsav\":" + String(invSet.flagPwrSav ? "true" : "false") + ",";
  j += "\"lcd\":" + String(invSet.flagLcd ? "true" : "false") + "},";

  j += "\"cfg\":{";
  j += "\"auto\":" + String(cfg.autoManage ? "true" : "false") + ",";
  j += "\"socGrid\":" + String(cfg.socToGrid, 1) + ",";
  j += "\"socBat\":" + String(cfg.socToBattery, 1) + ",";
  j += "\"maxSoc\":" + String(cfg.maxSocLimit, 1) + ",";
  j += "\"socCap\":" + String(socCapActive ? "true" : "false") + ",";
  j += "\"socChgOn\":" + String(cfg.socGridChgOn, 1) + ",";
  j += "\"socChgOff\":" + String(cfg.socGridChgOff, 1) + ",";
  j += "\"taper1\":" + String(cfg.cellTaper1V, 2) + ",";
  j += "\"taper2\":" + String(cfg.cellTaper2V, 2) + ",";
  j += "\"maxA\":" + String(cfg.maxChargeAmp) + ",";
  j += "\"midA\":" + String(cfg.midChargeAmp) + ",";
  j += "\"balA\":" + String(cfg.balChargeAmp) + ",";
  j += "\"bulkV\":" + String(cfg.bulkVoltage, 1) + ",";
  j += "\"floatV\":" + String(cfg.floatVoltage, 1) + ",";
  j += "\"tWarn\":" + String(cfg.invTempWarn) + ",";
  j += "\"tHigh\":" + String(cfg.invTempHigh) + ",";
  j += "\"tByp\":" + String(cfg.invTempBypass) + "},";
  j += "\"mqtt\":" + String(mqttClient.connected() ? "true" : "false") + ",";
  j += "\"up\":" + String(millis() / 1000);
  j += "}";
  return j;
}

// ======================= 9. WEB ARAYÜZÜ (PWA / MOBİL UYUMLU) ===========
#include "web_ui.h"

// ======================= 10. GERÇEK ZAMANLI MQTT (APK / 4.5G) ===========
String getParamVal(const String& s, const String& key) {
  String pat = key + "=";
  int p = s.indexOf(pat);
  if (p < 0) return "";
  p += pat.length();
  int e = s.indexOf('&', p);
  return (e < 0) ? s.substring(p) : s.substring(p, e);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  if (msg.length() == 0) return;

  if (t == MQTT_T_CMD) {
    if (msg == "APPLY_LIFEPO4") {
      applyFullLiFePO4Setup(true);
    } else if (msg == "RESET_STATS") {
      resetEnergyStore();
      cachedStatsJson = buildStatsJson();
      mqttClient.publish(MQTT_T_STATS, cachedStatsJson.c_str(), true);
      inv.lastCmdSent = "[APK] Enerji sayaclari sifirlandi";
    } else if (msg.startsWith("BMS:")) {
      String r = handleBmsControlCmd(msg);
      inv.lastCmdSent = "[BMS] " + r;
    } else if (msg.length() >= 3 && msg.length() <= 18) {
      String r, used = msg;
      if (msg.startsWith("MCHGC") || (msg.startsWith("MNCHGC") && msg.length() <= 9)) {
        int ampVal = msg.substring(msg.startsWith("MN") ? 6 : 5).toInt();
        r = setInverterChargeAmp(ampVal, &used);
      } else {
        r = sendInverterCommand(msg, 800);
      }
      inv.lastCmdSent = "[APK] " + used + " -> " + r;
    }
    delay(200);
    pollInverterSettings();
    // Komut sonrası anında güncel durumu APK'ya geri bas
    String fresh = buildStatusJson();
    if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(150)) == pdTRUE) {
      cachedStatusJson = fresh;
      xSemaphoreGive(jsonMutex);
    }
    mqttClient.publish(MQTT_T_STATUS, fresh.c_str(), true);
  } else if (t == MQTT_T_CFG) {
    String v;
    if ((v = getParamVal(msg, "auto")).length())   cfg.autoManage    = (v == "1");
    if ((v = getParamVal(msg, "socG")).length())   cfg.socToGrid     = v.toFloat();
    if ((v = getParamVal(msg, "socB")).length())   cfg.socToBattery  = v.toFloat();
    if ((v = getParamVal(msg, "maxS")).length())   cfg.maxSocLimit   = constrain(v.toFloat(), 50.0f, 100.0f);
    if ((v = getParamVal(msg, "chgOn")).length())  cfg.socGridChgOn  = v.toFloat();
    if ((v = getParamVal(msg, "chgOff")).length()) cfg.socGridChgOff = v.toFloat();
    if ((v = getParamVal(msg, "tap1")).length())   cfg.cellTaper1V   = v.toFloat();
    if ((v = getParamVal(msg, "tap2")).length())   cfg.cellTaper2V   = v.toFloat();
    if ((v = getParamVal(msg, "maxA")).length())   cfg.maxChargeAmp  = v.toInt();
    if ((v = getParamVal(msg, "midA")).length())   cfg.midChargeAmp  = v.toInt();
    if ((v = getParamVal(msg, "balA")).length())   cfg.balChargeAmp  = v.toInt();
    if ((v = getParamVal(msg, "bulkV")).length())  cfg.bulkVoltage   = constrain(v.toFloat(), 52.0f, 57.2f);
    if ((v = getParamVal(msg, "floatV")).length()) cfg.floatVoltage  = constrain(v.toFloat(), 51.0f, cfg.bulkVoltage);
    if ((v = getParamVal(msg, "tWarn")).length())  cfg.invTempWarn   = constrain(v.toInt(), 45, 78);
    if ((v = getParamVal(msg, "tHigh")).length())  cfg.invTempHigh   = constrain(v.toInt(), cfg.invTempWarn + 2, 84);
    if ((v = getParamVal(msg, "tByp")).length())   cfg.invTempBypass = constrain(v.toInt(), cfg.invTempHigh + 2, 90);

    prefs.putBool("auto", cfg.autoManage);
    prefs.putFloat("socG", cfg.socToGrid);
    prefs.putFloat("socB", cfg.socToBattery);
    prefs.putFloat("maxS", cfg.maxSocLimit);
    prefs.putFloat("chgOn", cfg.socGridChgOn);
    prefs.putFloat("chgOff", cfg.socGridChgOff);
    prefs.putFloat("tap1", cfg.cellTaper1V);
    prefs.putFloat("tap2", cfg.cellTaper2V);
    prefs.putInt("maxA", cfg.maxChargeAmp);
    prefs.putInt("midA", cfg.midChargeAmp);
    prefs.putInt("balA", cfg.balChargeAmp);
    prefs.putFloat("bulkV", cfg.bulkVoltage);
    prefs.putFloat("floatV", cfg.floatVoltage);
    prefs.putInt("tWarn", cfg.invTempWarn);
    prefs.putInt("tHigh", cfg.invTempHigh);
    prefs.putInt("tByp", cfg.invTempBypass);

    inv.lastCmdSent = "[APK] Kurallar kaydedildi (Ust Sinir: %" + String(cfg.maxSocLimit, 0) + ")";
    forceSmartCheck = true; // Yeni kuralı hemen uygula
    String fresh = buildStatusJson();
    if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(150)) == pdTRUE) {
      cachedStatusJson = fresh;
      xSemaphoreGive(jsonMutex);
    }
    mqttClient.publish(MQTT_T_STATUS, fresh.c_str(), true);
  }
}

void maintainMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqttClient.connected()) {
    if (millis() - lastMqttTryMs >= 5000) {
      lastMqttTryMs = millis();
      String cid = "ESP32_DalyInv_" + String((uint32_t)ESP.getEfuseMac(), HEX);
      if (mqttClient.connect(cid.c_str())) {
        mqttClient.subscribe(MQTT_T_CMD);
        mqttClient.subscribe(MQTT_T_CFG);
        Serial.println("MQTT Baglandi -> broker.emqx.io");
      }
    }
  } else {
    mqttClient.loop();
    if (millis() - lastMqttPubMs >= 3000) {
      lastMqttPubMs = millis();
      String out;
      if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        out = cachedStatusJson;
        xSemaphoreGive(jsonMutex);
      } else {
        out = buildStatusJson();
      }
      mqttClient.publish(MQTT_T_STATUS, out.c_str(), true);
    }
    if (millis() - lastStatsPubMs >= 12000) {
      lastStatsPubMs = millis();
      String sout;
      if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sout = cachedStatsJson;
        xSemaphoreGive(jsonMutex);
      } else {
        sout = buildStatsJson();
      }
      mqttClient.publish(MQTT_T_STATS, sout.c_str(), true);
    }
  }
}

// ======================= 11. KURULUM (SETUP) VE ANA DÖNGÜ (LOOP) ========
void setup() {
  Serial.begin(115200);
  // 1) İnverter RS232 Portunu Başlat (MAX3232 -> RX=GPIO19, TX=GPIO18 @ 2400)
  Serial2.begin(invBaud, SERIAL_8N1, invRxPin, invTxPin);
  // 2) Daly BMS RS485 Portunu Başlat (HW-519-R4 -> GPIO16/17 otomatik yön algılama @ 9600)
  bmsMutex = xSemaphoreCreateMutex();
  initBmsRs485AutoPins();

  prefs.begin("bmsbridge2", false);
  cfg.autoManage    = prefs.getBool("auto", true);
  cfg.socToGrid     = prefs.getFloat("socG", 20.0f);
  cfg.socToBattery  = prefs.getFloat("socB", 30.0f);
  cfg.maxSocLimit   = prefs.getFloat("maxS", 90.0f);
  cfg.socGridChgOn  = prefs.getFloat("chgOn", 12.0f);
  cfg.socGridChgOff = prefs.getFloat("chgOff", 20.0f);
  cfg.cellTaper1V   = prefs.getFloat("tap1", 3.45f);
  cfg.cellTaper2V   = prefs.getFloat("tap2", 3.49f);
  cfg.maxChargeAmp  = prefs.getInt("maxA", 100);
  cfg.midChargeAmp  = prefs.getInt("midA", 40);
  cfg.balChargeAmp  = prefs.getInt("balA", 10);
  cfg.bulkVoltage   = prefs.getFloat("bulkV", 56.8f);
  cfg.floatVoltage  = prefs.getFloat("floatV", 56.0f);
  cfg.invTempWarn   = prefs.getInt("tWarn", 65);
  cfg.invTempHigh   = prefs.getInt("tHigh", 72);
  cfg.invTempBypass = prefs.getInt("tByp", 78);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Anında 2ms tepki süresi - Telefon bağlantısı asla zaman aşımına uğramaz!
  WiFi.setAutoReconnect(true);
  // Sabit IP Yapılandırması: 192.168.1.140
  IPAddress staticIP(192, 168, 1, 140);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns1(8, 8, 8, 8);
  IPAddress dns2(1, 1, 1, 1);
  WiFi.config(staticIP, gateway, subnet, dns1, dns2);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Wi-Fi agina baglaniliyor: ");
  Serial.print(WIFI_SSID);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 30) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Baglandi! ESP32 IP Adresi: http://");
    Serial.println(WiFi.localIP());
    MDNS.begin("akilli-inverter"); // http://akilli-inverter.local adresinden girilebilir
  }

  // Web Sunucu Rotaları (CORS açık - Mobil APK / PWA doğrudan bağlanabilir)
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/api/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String out;
    if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      out = cachedStatusJson;
      xSemaphoreGive(jsonMutex);
    } else {
      out = buildStatusJson();
    }
    server.send(200, "application/json; charset=utf-8", out);
  });

  server.on("/api/stats", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String out;
    if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      out = cachedStatsJson;
      xSemaphoreGive(jsonMutex);
    } else {
      out = buildStatsJson();
    }
    server.send(200, "application/json; charset=utf-8", out);
  });

  server.on("/api/stats_reset", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    resetEnergyStore();
    cachedStatsJson = buildStatsJson();
    server.send(200, "application/json", "{\"reset\":true}");
  });

  // HW-519-R4 RS485 Canli Teshis Araci (Pin 16/17 3.3V kontrolu + 0xA5 0x40 / 0x80 / Modbus 0xD2 testi)
  server.on("/api/bms_probe", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (bmsMutex) xSemaphoreTake(bmsMutex, pdMS_TO_TICKS(3000));
    Serial1.end();
    delay(20);
    pinMode(16, INPUT_PULLDOWN);
    pinMode(17, INPUT_PULLDOWN);
    delay(6);
    int idle16 = digitalRead(16);
    int idle17 = digitalRead(17);
    pinMode(16, INPUT);
    pinMode(17, INPUT);

    String rep = "Idle3.3V: GPIO16=" + String(idle16) + " GPIO17=" + String(idle17) + "\n";

    auto testSeq = [&](const char* lbl, int rxP, int txP, int baud, const uint8_t* pkt, size_t len) {
      Serial1.end();
      Serial1.begin(baud, SERIAL_8N1, rxP, txP);
      delay(25);
      while (Serial1.available()) Serial1.read();
      Serial1.write(pkt, len);
      Serial1.flush();
      uint32_t t0 = millis();
      String hx = "";
      int cnt = 0;
      while (millis() - t0 < 400) {
        while (Serial1.available()) {
          uint8_t b = (uint8_t)Serial1.read();
          char bh[4];
          snprintf(bh, sizeof(bh), "%02X ", b);
          hx += bh;
          cnt++;
          t0 = millis();
        }
        delay(3);
      }
      rep += String(lbl) + " (RX=" + String(rxP) + ",TX=" + String(txP) + "@" + String(baud) + ") -> [" + String(cnt) + "B] " + hx + "\n";
      if (cnt >= 13 && hx.startsWith("A5 01")) {
        bmsRxPin = rxP;
        bmsTxPin = txP;
        bmsBaud  = baud;
        bmsDiagInfo = "RS485 BAGLI (RX=" + String(rxP) + ",TX=" + String(txP) + ")";
      }
    };

    const uint8_t p40[13]  = {0xA5,0x40,0x90,0x08,0,0,0,0,0,0,0,0,0x7D};
    const uint8_t p80[13]  = {0xA5,0x80,0x90,0x08,0,0,0,0,0,0,0,0,0xBD};
    const uint8_t pMod[8]  = {0xD2,0x03,0x00,0x00,0x00,0x04,0x57,0xA8};

    // Aktif 3.3V pini hangisiyse once onu RX olarak test et, sonra digerini
    int r1 = (idle17 == HIGH && idle16 == LOW) ? 17 : 16;
    int t1 = (r1 == 16) ? 17 : 16;

    testSeq("Daly-0x40", r1, t1, 9600, p40, 13);
    testSeq("Daly-0x80", r1, t1, 9600, p80, 13);
    testSeq("Daly-Mod ", r1, t1, 9600, pMod, 8);
    // Diger yonu de (t1/r1) dene eger ilk yonde veri gelmediyse
    if (rep.indexOf("A5 01") < 0) {
      testSeq("Swap-0x40", t1, r1, 9600, p40, 13);
      testSeq("Swap-0x80", t1, r1, 9600, p80, 13);
    }

    Serial1.end();
    Serial1.begin(bmsBaud, SERIAL_8N1, bmsRxPin, bmsTxPin);
    if (bmsMutex) xSemaphoreGive(bmsMutex);
    server.send(200, "text/plain", rep);
  });

  // Tum WatchPower / Voltronic (PI30, PI18, Megatec) ve Modbus protokollerini otomatik test eder
  server.on("/api/hunt", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String out = "";

    auto tryRaw = [&](const char* label, int baud, const uint8_t* data, size_t len) {
      Serial2.end();
      Serial2.begin(baud, SERIAL_8N1, invRxPin, invTxPin);
      delay(25);
      while (Serial2.available()) Serial2.read();
      Serial2.write(data, len);
      Serial2.flush();
      uint32_t t0 = millis();
      String hx = "";
      String tx = "";
      int cnt = 0;
      while (millis() - t0 < 550) {
        while (Serial2.available()) {
          uint8_t b = Serial2.read();
          char buf[4];
          snprintf(buf, sizeof(buf), "%02X ", b);
          hx += buf;
          if (b >= 32 && b <= 126) tx += (char)b;
          cnt++;
          t0 = millis();
        }
        delay(3);
      }
      if (cnt > 0) {
        out += String(label) + " @" + String(baud) + " -> [" + String(cnt) + "B] HEX:" + hx + " TXT:" + tx + "\n";
      }
    };

    auto tryVoltronic = [&](const char* cmd, int baud, bool withCrc) {
      uint8_t buf[32];
      size_t n = strlen(cmd);
      memcpy(buf, cmd, n);
      if (withCrc) {
        uint16_t crc = calcVoltronicCRC(buf, n);
        buf[n++] = (uint8_t)(crc >> 8);
        buf[n++] = (uint8_t)(crc & 0xFF);
      }
      buf[n++] = '\r';
      String lbl = String(cmd) + (withCrc ? "+CRC" : "-noCRC");
      tryRaw(lbl.c_str(), baud, buf, n);
    };

    const int bauds[] = {2400, 9600, 19200, 4800};
    for (int b : bauds) {
      tryVoltronic("QPI", b, true);
      tryVoltronic("QPIGS", b, true);
      tryVoltronic("QMOD", b, true);
      tryVoltronic("QPI", b, false);
      tryVoltronic("^P003PI", b, false);
      tryVoltronic("^P005GS", b, false);
      tryVoltronic("^P003PI", b, true);
      tryVoltronic("Q1", b, false);
    }

    // Modbus RTU denemeleri (9600 ve 2400)
    const uint8_t mb03[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
    const uint8_t mb04[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x02, 0x71, 0xCB};
    const uint8_t mbSrne[] = {0x01, 0x03, 0x01, 0x00, 0x00, 0x02, 0xC5, 0xF7};
    tryRaw("Modbus03", 9600, mb03, sizeof(mb03));
    tryRaw("Modbus04", 9600, mb04, sizeof(mb04));
    tryRaw("ModbusSRNE", 9600, mbSrne, sizeof(mbSrne));
    tryRaw("Modbus03", 2400, mb03, sizeof(mb03));

    Serial2.end();
    Serial2.begin(invBaud, SERIAL_8N1, invRxPin, invTxPin);

    if (out.length() == 0) out = "HICBIR PROTOKOLDEN (PI30/PI18/Megatec/Modbus @ 2400-19200) YANIT GELMEDI (0 BYTE)";
    server.send(200, "text/plain", out);
  });

  server.on("/api/scan_inv", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String rep = scanInverterPins();
    server.send(200, "application/json", "{\"report\":\"" + rep + "\"}");
  });

  // Gelismis Canli Port/Baud/Inversion/Raw-Hex Test Araci
  server.on("/api/probe", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    int rxP   = server.hasArg("rx")   ? server.arg("rx").toInt()   : invRxPin;
    int txP   = server.hasArg("tx")   ? server.arg("tx").toInt()   : invTxPin;
    int baud  = server.hasArg("baud") ? server.arg("baud").toInt() : 2400;
    bool invS = server.hasArg("inv")  ? (server.arg("inv") == "1") : false;
    String cmd = server.hasArg("c")   ? server.arg("c")            : "QPI";

    Serial2.end();
    delay(20);
    // Once pinlerin bosta (idle) voltaj durumunu olc
    pinMode(rxP, INPUT_PULLDOWN);
    pinMode(txP, INPUT_PULLDOWN);
    delay(5);
    int rxIdle = digitalRead(rxP);
    int txIdle = digitalRead(txP);
    pinMode(rxP, INPUT);
    pinMode(txP, INPUT);

    Serial2.begin(baud, SERIAL_8N1, rxP, txP, invS);
    delay(30);
    while (Serial2.available()) Serial2.read();

    uint16_t crc = calcVoltronicCRC((const uint8_t*)cmd.c_str(), cmd.length());
    Serial2.print(cmd);
    Serial2.write((uint8_t)(crc >> 8));
    Serial2.write((uint8_t)(crc & 0xFF));
    Serial2.write('\r');
    Serial2.flush();

    String hexOut = "";
    String txtOut = "";
    uint32_t t0 = millis();
    int cnt = 0;
    while (millis() - t0 < 1200) {
      while (Serial2.available()) {
        uint8_t b = Serial2.read();
        char hx[4];
        snprintf(hx, sizeof(hx), "%02X ", b);
        hexOut += hx;
        if (b >= 32 && b <= 126) txtOut += (char)b;
        cnt++;
        t0 = millis(); // veri geldikce bekle
      }
      delay(5);
    }

    // Geri eski ayarlara don (eger basarili olduysa kalici kilitle)
    if (txtOut.startsWith("(")) {
      invRxPin = rxP;
      invTxPin = txP;
      invBaud  = baud;
    } else {
      Serial2.end();
      Serial2.begin(invBaud, SERIAL_8N1, invRxPin, invTxPin);
    }

    String res = "{";
    res += "\"rx\":" + String(rxP) + ",\"tx\":" + String(txP) + ",\"baud\":" + String(baud) + ",\"inv\":" + String(invS ? 1 : 0) + ",";
    res += "\"rxIdle\":" + String(rxIdle) + ",\"txIdle\":" + String(txIdle) + ",";
    res += "\"bytes\":" + String(cnt) + ",\"hex\":\"" + hexOut + "\",\"txt\":\"" + txtOut + "\"}";
    server.send(200, "application/json", res);
  });

  server.on("/api/cmd", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("c")) {
      String c = server.arg("c");
      if (c.startsWith("BMS:")) {
        String r = handleBmsControlCmd(c);
        inv.lastCmdSent = "[BMS] " + r;
        server.send(200, "application/json", "{\"res\":\"" + r + "\"}");
        return;
      }
      String r, used = c;
      if (c.startsWith("MCHGC") || (c.startsWith("MNCHGC") && c.length() <= 9)) {
        int ampVal = c.substring(c.startsWith("MN") ? 6 : 5).toInt();
        r = setInverterChargeAmp(ampVal, &used);
      } else {
        r = sendInverterCommand(c);
      }
      inv.lastCmdSent = used + " -> " + r;
      delay(150);
      pollInverterSettings();
      server.send(200, "application/json", "{\"res\":\"" + r + "\"}");
    } else {
      server.send(400, "text/plain", "Eksik komut");
    }
  });

  server.on("/api/apply_lifepo4", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String log = applyFullLiFePO4Setup(true);
    pollInverterSettings();
    server.send(200, "application/json", "{\"ok\":true,\"log\":\"" + log + "\"}");
  });

  server.on("/api/cfg", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("auto"))   cfg.autoManage    = (server.arg("auto") == "1");
    if (server.hasArg("socG"))   cfg.socToGrid     = server.arg("socG").toFloat();
    if (server.hasArg("socB"))   cfg.socToBattery  = server.arg("socB").toFloat();
    if (server.hasArg("maxS"))   cfg.maxSocLimit   = constrain(server.arg("maxS").toFloat(), 50.0f, 100.0f);
    if (server.hasArg("chgOn"))  cfg.socGridChgOn  = server.arg("chgOn").toFloat();
    if (server.hasArg("chgOff")) cfg.socGridChgOff = server.arg("chgOff").toFloat();
    if (server.hasArg("tap1"))   cfg.cellTaper1V   = server.arg("tap1").toFloat();
    if (server.hasArg("tap2"))   cfg.cellTaper2V   = server.arg("tap2").toFloat();
    if (server.hasArg("maxA"))   cfg.maxChargeAmp  = server.arg("maxA").toInt();
    if (server.hasArg("midA"))   cfg.midChargeAmp  = server.arg("midA").toInt();
    if (server.hasArg("balA"))   cfg.balChargeAmp  = server.arg("balA").toInt();
    if (server.hasArg("bulkV"))  cfg.bulkVoltage   = constrain(server.arg("bulkV").toFloat(), 52.0f, 57.2f);
    if (server.hasArg("floatV")) cfg.floatVoltage  = constrain(server.arg("floatV").toFloat(), 51.0f, cfg.bulkVoltage);
    if (server.hasArg("tWarn"))  cfg.invTempWarn   = constrain(server.arg("tWarn").toInt(), 45, 78);
    if (server.hasArg("tHigh"))  cfg.invTempHigh   = constrain(server.arg("tHigh").toInt(), cfg.invTempWarn + 2, 84);
    if (server.hasArg("tByp"))   cfg.invTempBypass = constrain(server.arg("tByp").toInt(), cfg.invTempHigh + 2, 90);

    prefs.putBool("auto", cfg.autoManage);
    prefs.putFloat("socG", cfg.socToGrid);
    prefs.putFloat("socB", cfg.socToBattery);
    prefs.putFloat("maxS", cfg.maxSocLimit);
    prefs.putFloat("chgOn", cfg.socGridChgOn);
    prefs.putFloat("chgOff", cfg.socGridChgOff);
    prefs.putFloat("tap1", cfg.cellTaper1V);
    prefs.putFloat("tap2", cfg.cellTaper2V);
    prefs.putInt("maxA", cfg.maxChargeAmp);
    prefs.putInt("midA", cfg.midChargeAmp);
    prefs.putInt("balA", cfg.balChargeAmp);
    prefs.putFloat("bulkV", cfg.bulkVoltage);
    prefs.putFloat("floatV", cfg.floatVoltage);
    prefs.putInt("tWarn", cfg.invTempWarn);
    prefs.putInt("tHigh", cfg.invTempHigh);
    prefs.putInt("tByp", cfg.invTempBypass);
    forceSmartCheck = true;
    server.send(200, "application/json", "{\"saved\":true}");
  });

  // Kablosuz (Wi-Fi üzerinden) Yazılım Güncelleme (Bir daha USB veya BOOT tuşu gerekmez!)
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(500);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Update.begin(UPDATE_SIZE_UNKNOWN);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      Update.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });

  invMutex  = xSemaphoreCreateMutex();
  jsonMutex = xSemaphoreCreateMutex();
  initEnergyStore();
  cachedStatusJson = buildStatusJson();
  cachedStatsJson  = buildStatsJson();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(4096);
  mqttClient.setCallback(mqttCallback);

  server.begin();

  // Çekirdek 0: BMS + İnverter + Enerji Sayaçları + QPIRI/QFLAG + Gerçek Zamanlı MQTT Görevi
  xTaskCreatePinnedToCore([](void* pv) {
    for (;;) {
      maintainMqtt();
      if (millis() - lastBmsPollMs >= 3000) {
        pollDalyBMS();
        if (bms.connected && (!bmsSet.valid || (millis() - lastBmsSetPollMs >= 15000))) {
          pollDalySettings();
          lastBmsSetPollMs = millis();
        }
        lastBmsPollMs = millis();
      }
      maintainMqtt();
      if (millis() - lastInvPollMs >= 3500) {
        pollInverter();
        updateEnergyStats(inv.connected, inv.pvPowerWatt, inv.acLoadWatt,
                          inv.workMode.indexOf("(L)") >= 0,
                          bms.connected, bms.totalVoltage, bms.current);
        if (!invSet.valid || (millis() - lastQpiriPollMs >= 12000)) {
          pollInverterSettings();
          lastQpiriPollMs = millis();
        }
        runSmartBmsControl();
        lastInvPollMs = millis();
      }
      String fresh = buildStatusJson();
      String freshStats = buildStatsJson();
      if (jsonMutex && xSemaphoreTake(jsonMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        cachedStatusJson = fresh;
        cachedStatsJson  = freshStats;
        xSemaphoreGive(jsonMutex);
      }
      maintainMqtt();
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }, "BmsInvWorker", 8192, nullptr, 1, nullptr, 0);
}

void loop() {
  // Çekirdek 1: %100 sadece Web Arayüzüne ve telefon bağlantılarına ayrıldı!
  server.handleClient();
  delay(2);
}

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

struct DayEnergy {
  uint8_t day;
  uint8_t month;
  float pvKwh;       // ☀️ Panel Üretimi (kWh)
  float loadKwh;     // 🏠 Ev Tüketimi (kWh)
  float batChgKwh;   // 🔋 Aküye Dolan Şarj (kWh)
  float batDisKwh;   // 🔋 Aküden Kullanılan (kWh)
  float gridOutKwh;  // 🔌 Şebeke Çıkışı (kWh)
  float gridChgKwh;  // ⚡ Şebeke Şarjı (kWh)
};

// Eski V1 formatı (NVS'ten mevcut 30 günlük geçmişi kaybetmeden V2'ye taşımak için)
struct EnergyStoreV1 {
  uint32_t magic;
  DayEnergy days[30];
  float hrPv[24];
  float hrLoad[24];
  float hrGrid[24];
};

// Yeni V2 formatı: Saatlik Akü Dolum (hrChg) ve Akü Kullanım (hrDis) dahil!
struct EnergyStore {
  uint32_t magic;
  DayEnergy days[30]; // days[0] = bugün, days[1..29] = önceki 29 gün
  float hrPv[24];     // Bugün saatlik solar üretim (kWh)
  float hrLoad[24];   // Bugün saatlik ev tüketimi (kWh)
  float hrChg[24];    // Bugün saatlik akü dolum (kWh)
  float hrDis[24];    // Bugün saatlik akü kullanım (kWh)
  float hrGrid[24];   // Bugün saatlik toplam şebeke (çıkış + şarj, kWh)
};

static const uint32_t ENERGY_MAGIC_V1 = 0xD41B3001;
static const uint32_t ENERGY_MAGIC    = 0xD41B3002;
EnergyStore eStore;
Preferences ePrefs;
uint32_t lastEnergyTickMs = 0;
uint32_t lastEnergySaveMs = 0;
uint32_t lastStatsPubMs   = 0;
String   cachedStatsJson  = "{}";

void saveEnergyStore() {
  ePrefs.putBytes("data", &eStore, sizeof(eStore));
  lastEnergySaveMs = millis();
}

void initEnergyStore() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.google.com", "tr.pool.ntp.org");
  ePrefs.begin("energy30d", false);
  memset(&eStore, 0, sizeof(eStore));

  size_t len = ePrefs.getBytesLength("data");
  if (len == sizeof(EnergyStore)) {
    ePrefs.getBytes("data", &eStore, sizeof(EnergyStore));
  } else if (len == sizeof(EnergyStoreV1)) {
    // Mevcut V1 verilerini (örn. 24/11, 25/11) sıfırlamadan V2'ye taşı!
    EnergyStoreV1 oldS;
    memset(&oldS, 0, sizeof(oldS));
    ePrefs.getBytes("data", &oldS, sizeof(EnergyStoreV1));
    if (oldS.magic == ENERGY_MAGIC_V1) {
      eStore.magic = ENERGY_MAGIC;
      memcpy(eStore.days, oldS.days, sizeof(eStore.days));
      memcpy(eStore.hrPv, oldS.hrPv, sizeof(eStore.hrPv));
      memcpy(eStore.hrLoad, oldS.hrLoad, sizeof(eStore.hrLoad));
      memcpy(eStore.hrGrid, oldS.hrGrid, sizeof(eStore.hrGrid));
      // Eski gece PV gürültüsü (<0.03 kWh gece saatleri) temizliği
      for (int h = 0; h < 24; h++) {
        if ((h <= 5 || h >= 19) && eStore.hrPv[h] < 0.035f) eStore.hrPv[h] = 0.0f;
      }
      saveEnergyStore();
    }
  }

  if (eStore.magic != ENERGY_MAGIC) {
    memset(&eStore, 0, sizeof(eStore));
    eStore.magic = ENERGY_MAGIC;
    // NTP gelene kadar day=0 bırak: sahte tarihli boş gün üretme!
    saveEnergyStore();
  }
  lastEnergyTickMs = millis();
  lastEnergySaveMs = millis();
}

void resetEnergyStore() {
  struct tm ti;
  uint8_t d = 0, m = 0;
  if (getLocalTime(&ti, 50) && ti.tm_year >= 120) {
    d = ti.tm_mday;
    m = ti.tm_mon + 1;
  }
  memset(&eStore, 0, sizeof(eStore));
  eStore.magic = ENERGY_MAGIC;
  eStore.days[0].day   = d;
  eStore.days[0].month = m;
  saveEnergyStore();
}

void updateEnergyStats(bool invOk, int pvW, int loadW, bool isLineMode, bool bmsOk, float bmsV, float bmsA) {
  uint32_t now = millis();
  uint32_t dtMs = now - lastEnergyTickMs;
  if (dtMs < 500 || dtMs > 60000) {
    lastEnergyTickMs = now;
    return;
  }
  lastEnergyTickMs = now;

  // Gerçek Türkiye saatini (UTC+3) doğrula; NTP hazır değilse yanlış saate (12:00) veya sahte güne yazma!
  struct tm ti;
  bool ntpOk = (getLocalTime(&ti, 15) && ti.tm_year >= 120);
  int curHour = -1;
  if (ntpOk) {
    curHour = ti.tm_hour;
    uint8_t curDay = ti.tm_mday;
    uint8_t curMon = ti.tm_mon + 1;
    if (eStore.days[0].day == 0) {
      eStore.days[0].day   = curDay;
      eStore.days[0].month = curMon;
      saveEnergyStore();
    } else if (eStore.days[0].day != curDay || eStore.days[0].month != curMon) {
      for (int i = 29; i >= 1; i--) eStore.days[i] = eStore.days[i - 1];
      memset(&eStore.days[0], 0, sizeof(DayEnergy));
      eStore.days[0].day   = curDay;
      eStore.days[0].month = curMon;
      memset(eStore.hrPv, 0, sizeof(eStore.hrPv));
      memset(eStore.hrLoad, 0, sizeof(eStore.hrLoad));
      memset(eStore.hrChg, 0, sizeof(eStore.hrChg));
      memset(eStore.hrDis, 0, sizeof(eStore.hrDis));
      memset(eStore.hrGrid, 0, sizeof(eStore.hrGrid));
      saveEnergyStore();
    }
  }

  float dtH = dtMs / 3600000.0f;
  float pvKwhInc   = (invOk && pvW >= 15) ? (pvW / 1000.0f) * dtH   : 0.0f;
  float loadKwhInc = (invOk && loadW > 0) ? (loadW / 1000.0f) * dtH : 0.0f;
  float bmsW       = bmsOk ? (bmsV * bmsA) : 0.0f;
  float chgKwhInc  = (bmsW > 8.0f)  ? (bmsW / 1000.0f) * dtH  : 0.0f;
  float disKwhInc  = (bmsW < -8.0f) ? (-bmsW / 1000.0f) * dtH : 0.0f;

  float gridOutInc = 0.0f;
  float gridChgInc = 0.0f;
  if (invOk && isLineMode) {
    gridOutInc = loadKwhInc;
    // Şebekeden aküye basılan güç (BMS'e giren güçten güneş payı düşülerek)
    float pvNetW = (pvW >= 15) ? (pvW * 0.92f) : 0.0f;
    float extraChgW = bmsW - pvNetW;
    if (extraChgW > 20.0f) gridChgInc = (extraChgW / 1000.0f) * dtH;
  }

  eStore.days[0].pvKwh      += pvKwhInc;
  eStore.days[0].loadKwh    += loadKwhInc;
  eStore.days[0].batChgKwh  += chgKwhInc;
  eStore.days[0].batDisKwh  += disKwhInc;
  eStore.days[0].gridOutKwh += gridOutInc;
  eStore.days[0].gridChgKwh += gridChgInc;

  if (curHour >= 0 && curHour < 24) {
    eStore.hrPv[curHour]   += pvKwhInc;
    eStore.hrLoad[curHour] += loadKwhInc;
    eStore.hrChg[curHour]  += chgKwhInc;
    eStore.hrDis[curHour]  += disKwhInc;
    eStore.hrGrid[curHour] += (gridOutInc + gridChgInc);
  }

  // Her 5 dakikada bir NVS Flash'a yedekle (elektrik kesilse bile kaybolmaz)
  if (now - lastEnergySaveMs >= 300000) {
    saveEnergyStore();
  }
}

String buildStatsJson() {
  struct tm ti;
  char dateBuf[24] = "--.--.---- --:--";
  int curHr = -1;
  if (getLocalTime(&ti, 20) && ti.tm_year >= 120) {
    curHr = ti.tm_hour;
    snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%04d %02d:%02d",
             ti.tm_mday, ti.tm_mon + 1, ti.tm_year + 1900, ti.tm_hour, ti.tm_min);
  }

  float mPv = 0, mLoad = 0, mChg = 0, mDis = 0, mGridOut = 0, mGridChg = 0;
  int daysCnt = 0;
  for (int i = 0; i < 30; i++) {
    if (eStore.days[i].day > 0) {
      daysCnt++;
      mPv      += eStore.days[i].pvKwh;
      mLoad    += eStore.days[i].loadKwh;
      mChg     += eStore.days[i].batChgKwh;
      mDis     += eStore.days[i].batDisKwh;
      mGridOut += eStore.days[i].gridOutKwh;
      mGridChg += eStore.days[i].gridChgKwh;
    }
  }

  String j = "{";
  j += "\"dt\":\"" + String(dateBuf) + "\",";
  j += "\"hr\":" + String(curHr) + ",";
  j += "\"daysCnt\":" + String(daysCnt) + ",";
  j += "\"today\":{";
  j += "\"pv\":"      + String(eStore.days[0].pvKwh, 3) + ",";
  j += "\"load\":"    + String(eStore.days[0].loadKwh, 3) + ",";
  j += "\"chg\":"     + String(eStore.days[0].batChgKwh, 3) + ",";
  j += "\"dis\":"     + String(eStore.days[0].batDisKwh, 3) + ",";
  j += "\"gridOut\":" + String(eStore.days[0].gridOutKwh, 3) + ",";
  j += "\"gridChg\":" + String(eStore.days[0].gridChgKwh, 3) + "},";

  j += "\"month\":{";
  j += "\"pv\":"      + String(mPv, 2) + ",";
  j += "\"load\":"    + String(mLoad, 2) + ",";
  j += "\"chg\":"     + String(mChg, 2) + ",";
  j += "\"dis\":"     + String(mDis, 2) + ",";
  j += "\"gridOut\":" + String(mGridOut, 2) + ",";
  j += "\"gridChg\":" + String(mGridChg, 2) + "},";

  // 30 günlük geçmiş (eskiden yeniye kronolojik: [tarih, pv, load, chg, dis, gridOut, gridChg])
  j += "\"days\":[";
  bool first = true;
  for (int i = 29; i >= 0; i--) {
    if (eStore.days[i].day > 0) {
      if (!first) j += ",";
      first = false;
      char lbl[8];
      snprintf(lbl, sizeof(lbl), "%02d/%02d", eStore.days[i].day, eStore.days[i].month);
      j += "[\"" + String(lbl) + "\"," +
           String(eStore.days[i].pvKwh, 2) + "," +
           String(eStore.days[i].loadKwh, 2) + "," +
           String(eStore.days[i].batChgKwh, 2) + "," +
           String(eStore.days[i].batDisKwh, 2) + "," +
           String(eStore.days[i].gridOutKwh, 2) + "," +
           String(eStore.days[i].gridChgKwh, 2) + "]";
    }
  }
  j += "],";

  // 24 saatlik bugünkü eğriler (Solar, Ev Yükü, Akü Dolum, Akü Kullanım, Şebeke)
  j += "\"hPv\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrPv[h], 2); if (h < 23) j += ","; }
  j += "],\"hLoad\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrLoad[h], 2); if (h < 23) j += ","; }
  j += "],\"hChg\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrChg[h], 2); if (h < 23) j += ","; }
  j += "],\"hDis\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrDis[h], 2); if (h < 23) j += ","; }
  j += "],\"hGrid\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrGrid[h], 2); if (h < 23) j += ","; }
  j += "]}";
  return j;
}

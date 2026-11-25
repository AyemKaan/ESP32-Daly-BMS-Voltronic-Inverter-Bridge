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

struct EnergyStore {
  uint32_t magic;
  DayEnergy days[30]; // days[0] = bugün, days[1..29] = önceki 29 gün
  float hrPv[24];     // Bugün saatlik solar (kWh)
  float hrLoad[24];   // Bugün saatlik ev yükü (kWh)
  float hrGrid[24];   // Bugün saatlik şebeke (kWh)
};

static const uint32_t ENERGY_MAGIC = 0xD41B3001;
EnergyStore eStore;
Preferences ePrefs;
uint32_t lastEnergyTickMs = 0;
uint32_t lastEnergySaveMs = 0;
uint32_t lastStatsPubMs   = 0;
String   cachedStatsJson  = "{}";

void initEnergyStore() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.google.com", "tr.pool.ntp.org");
  ePrefs.begin("energy30d", false);
  memset(&eStore, 0, sizeof(eStore));
  if (ePrefs.getBytesLength("data") == sizeof(eStore)) {
    ePrefs.getBytes("data", &eStore, sizeof(eStore));
  }
  if (eStore.magic != ENERGY_MAGIC) {
    memset(&eStore, 0, sizeof(eStore));
    eStore.magic = ENERGY_MAGIC;
    eStore.days[0].day   = 23;
    eStore.days[0].month = 11;
    ePrefs.putBytes("data", &eStore, sizeof(eStore));
  }
  lastEnergyTickMs = millis();
  lastEnergySaveMs = millis();
}

void saveEnergyStore() {
  ePrefs.putBytes("data", &eStore, sizeof(eStore));
  lastEnergySaveMs = millis();
}

void resetEnergyStore() {
  struct tm ti;
  uint8_t d = 23, m = 11;
  if (getLocalTime(&ti, 50)) { d = ti.tm_mday; m = ti.tm_mon + 1; }
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

  // Gerçek Türkiye saatini kontrol et; gün değiştiyse 30 günlük diziyi kaydır
  struct tm ti;
  int curHour = 12;
  if (getLocalTime(&ti, 20)) {
    curHour = ti.tm_hour;
    uint8_t curDay = ti.tm_mday;
    uint8_t curMon = ti.tm_mon + 1;
    if (eStore.days[0].day == 0) {
      eStore.days[0].day   = curDay;
      eStore.days[0].month = curMon;
    } else if (eStore.days[0].day != curDay || eStore.days[0].month != curMon) {
      for (int i = 29; i >= 1; i--) eStore.days[i] = eStore.days[i - 1];
      memset(&eStore.days[0], 0, sizeof(DayEnergy));
      eStore.days[0].day   = curDay;
      eStore.days[0].month = curMon;
      memset(eStore.hrPv, 0, sizeof(eStore.hrPv));
      memset(eStore.hrLoad, 0, sizeof(eStore.hrLoad));
      memset(eStore.hrGrid, 0, sizeof(eStore.hrGrid));
      saveEnergyStore();
    }
  }

  float dtH = dtMs / 3600000.0f;
  float pvKwhInc   = (invOk && pvW >= 15) ? (pvW / 1000.0f) * dtH   : 0.0f;
  float loadKwhInc = (invOk && loadW > 0) ? (loadW / 1000.0f) * dtH : 0.0f;
  float bmsW       = bmsOk ? (bmsV * bmsA) : 0.0f;
  float chgKwhInc  = (bmsW > 5.0f)  ? (bmsW / 1000.0f) * dtH  : 0.0f;
  float disKwhInc  = (bmsW < -5.0f) ? (-bmsW / 1000.0f) * dtH : 0.0f;

  float gridOutInc = 0.0f;
  float gridChgInc = 0.0f;
  if (invOk && isLineMode) {
    gridOutInc = loadKwhInc;
    float extraChgW = bmsW - (float)max(0, pvW);
    if (extraChgW > 15.0f) gridChgInc = (extraChgW / 1000.0f) * dtH;
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
    eStore.hrGrid[curHour] += (gridOutInc + gridChgInc);
  }

  // Her 10 dakikada bir NVS Flash'a yedekle (elektrik kesilse bile kaybolmaz)
  if (now - lastEnergySaveMs >= 600000) {
    saveEnergyStore();
  }
}

String buildStatsJson() {
  struct tm ti;
  char dateBuf[24] = "--.--.---- --:--";
  if (getLocalTime(&ti, 20)) {
    snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%04d %02d:%02d",
             ti.tm_mday, ti.tm_mon + 1, ti.tm_year + 1900, ti.tm_hour, ti.tm_min);
  }

  float mPv = 0, mLoad = 0, mChg = 0, mDis = 0, mGridOut = 0, mGridChg = 0;
  for (int i = 0; i < 30; i++) {
    if (eStore.days[i].day > 0) {
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

  // 30 günlük geçmiş (eskiden yeniye kronolojik)
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
           String(eStore.days[i].gridOutKwh, 2) + "," +
           String(eStore.days[i].gridChgKwh, 2) + "]";
    }
  }
  j += "],";

  // 24 saatlik bugünkü eğri
  j += "\"hPv\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrPv[h], 2); if (h < 23) j += ","; }
  j += "],\"hLoad\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrLoad[h], 2); if (h < 23) j += ","; }
  j += "],\"hGrid\":[";
  for (int h = 0; h < 24; h++) { j += String(eStore.hrGrid[h], 2); if (h < 23) j += ","; }
  j += "]}";
  return j;
}

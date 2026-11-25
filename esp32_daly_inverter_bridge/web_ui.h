#pragma once
const char INDEX_HTML[] PROGMEM = R"HTMLEND(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="theme-color" content="#090f1d">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<link rel="manifest" href="manifest.json">
<title>Akıllı İnverter & Daly BMS</title>
<script src="./mqtt.min.js"></script>
<script src="./mqtt_config.js"></script>
<script>if(typeof mqtt==='undefined'){document.write('<script src="https://unpkg.com/mqtt@5.10.1/dist/mqtt.min.js"><\/script>');}</script>
<style>
  :root{
    --bg:#090f1d;--card:#131c2e;--card2:#0f1728;--border:#24324d;
    --green:#10b981;--blue:#38bdf8;--amber:#f59e0b;--red:#ef4444;--purple:#a78bfa;
    --txt:#f8fafc;--sub:#94a3b8
  }
  *{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent;font-family:system-ui,-apple-system,'Segoe UI',Roboto,sans-serif}
  body{background:var(--bg);color:var(--txt);padding:12px 12px 76px 12px;max-width:740px;margin:0 auto;user-select:none}
  .hdr{display:flex;justify-content:space-between;align-items:center;background:linear-gradient(135deg,#162238,#0f172a);padding:14px 16px;border-radius:18px;border:1px solid var(--border);margin-bottom:12px;box-shadow:0 8px 24px rgba(0,0,0,.35)}
  .title{font-size:16.5px;font-weight:800;letter-spacing:-.2px}
  .sub{font-size:11px;color:var(--sub);margin-top:2px}
  .pills{display:flex;gap:5px;flex-wrap:wrap;justify-content:flex-end}
  .pill{padding:4px 9px;border-radius:99px;font-size:11px;font-weight:700;display:inline-flex;align-items:center;gap:4px}
  .ok{background:#064e3b;color:#6ee7b7;border:1px solid #05966966}
  .warn{background:#78350f;color:#fcd34d;border:1px solid #d9770666}
  .no{background:#7f1d1d;color:#fca5a5;border:1px solid #dc262666}
  .info{background:#1e3a5f;color:#7dd3fc;border:1px solid #0284c766}

  .flow{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:12px}
  .fcard{background:var(--card);border:1px solid var(--border);border-radius:16px;padding:13px}
  .fcard .lbl{font-size:11px;color:var(--sub);font-weight:600;text-transform:uppercase;letter-spacing:.4px}
  .fcard .val{font-size:23px;font-weight:800;margin-top:4px;line-height:1.15}
  .fcard .det{font-size:11.5px;color:var(--sub);margin-top:5px}

  .card{background:var(--card);border:1px solid var(--border);border-radius:16px;padding:15px;margin-bottom:12px}
  .card h3{font-size:12px;color:var(--sub);text-transform:uppercase;letter-spacing:.7px;margin-bottom:10px;display:flex;justify-content:space-between;align-items:center;gap:8px;flex-wrap:wrap}

  .soc-bar-bg{height:12px;background:#090f1d;border-radius:99px;overflow:hidden;border:1px solid var(--border);margin:8px 0 10px 0}
  .soc-bar-fill{height:100%;background:linear-gradient(90deg,#059669,#34d399);border-radius:99px;transition:width .5s ease}

  .row{display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid #24324d66;font-size:13px;gap:8px}
  .row:last-child{border-bottom:none}
  .row span{color:var(--sub)}

  .cells{display:grid;grid-template-columns:repeat(4,1fr);gap:7px;margin-top:6px}
  .cell{background:var(--card2);border:1px solid var(--border);border-radius:10px;padding:7px 4px;text-align:center}
  .cell .cn{font-size:10px;color:var(--sub)}
  .cell .cv{font-size:13px;font-weight:800;margin-top:2px;font-variant-numeric:tabular-nums}
  .cell .cb{height:4px;background:#1e293b;border-radius:4px;margin:5px 4px 0 4px;overflow:hidden}
  .cell .cbf{height:100%;background:var(--green);border-radius:4px}
  .cell.hi{border-color:var(--amber)}.cell.hi .cv{color:var(--amber)}.cell.hi .cbf{background:var(--amber)}
  .cell.lo{border-color:var(--blue)}.cell.lo .cv{color:var(--blue)}.cell.lo .cbf{background:var(--blue)}

  .sec-lbl{font-size:11.5px;color:#cbd5e1;margin:12px 0 6px 0;font-weight:700;display:flex;justify-content:space-between;align-items:center}
  .sec-lbl small{color:var(--green);font-weight:700}
  .btns{display:flex;flex-wrap:wrap;gap:6px}
  button{flex:1;min-width:62px;padding:10px 7px;border:1px solid #334155;border-radius:10px;background:#1e293b;color:#f8fafc;font-weight:700;font-size:12px;cursor:pointer;transition:transform .1s,background .15s}
  button:active{transform:scale(.96)}
  button.act{background:#059669;border-color:#34d399;box-shadow:0 0 12px #10b98144}
  button.blu{background:#0284c7;border-color:#38bdf8}
  button.org{background:#d97706;border-color:#f59e0b}
  button.pur{background:#6d28d9;border-color:#8b5cf6}
  button.sm{flex:0 0 auto;padding:7px 12px;min-width:56px}

  .v-edit{display:flex;align-items:center;gap:6px}
  .v-cur{font-size:12px;font-weight:800;color:var(--green);min-width:46px;text-align:right}
  input[type=number]{width:70px;padding:7px 8px;border-radius:8px;border:1px solid #334155;background:#090f1d;color:#fff;text-align:right;font-weight:700;font-size:13px}
  .switch{width:44px;height:24px;accent-color:var(--green);cursor:pointer}

  .plan-box{background:#0b1325;border:1px solid #1e3a5f;border-radius:12px;padding:11px 12px;margin-bottom:10px;font-size:12px;line-height:1.55;color:#cbd5e1}
  .plan-box b{color:#34d399}
  .plan-box .w{color:#fbbf24;font-weight:700}

  .sgrid{display:grid;grid-template-columns:repeat(3,1fr);gap:7px;margin-bottom:10px}
  .scard{background:var(--card2);border:1px solid var(--border);border-radius:11px;padding:9px 7px;text-align:center}
  .scard .sl{font-size:10px;color:var(--sub);font-weight:600}
  .scard .sv{font-size:15px;font-weight:800;margin-top:3px}
  .legend{display:flex;flex-wrap:wrap;gap:10px;font-size:11px;color:var(--sub);margin:8px 0 6px 0;justify-content:center}
  .lg{display:inline-flex;align-items:center;gap:4px}
  .dot{width:9px;height:9px;border-radius:3px;display:inline-block}
  .chart-wrap{background:var(--card2);border:1px solid var(--border);border-radius:12px;padding:8px;overflow-x:auto}
  canvas{display:block;width:100%;height:210px}

  .toast{position:fixed;bottom:18px;left:50%;transform:translateX(-50%);background:#065f46;color:#ecfdf5;padding:10px 18px;border-radius:99px;font-size:13px;font-weight:700;box-shadow:0 8px 25px rgba(0,0,0,.5);opacity:0;pointer-events:none;transition:opacity .25s;z-index:99;border:1px solid #10b981;white-space:nowrap}
  .toast.show{opacity:1}
</style>
</head>
<body>

<div class="hdr">
  <div>
    <div class="title">⚡ Akıllı İnverter &amp; Daly BMS</div>
    <div class="sub" id="subInfo">10×550W Solar • 48V 280Ah LiFePO4 • Bağlanıyor...</div>
  </div>
  <div class="pills">
    <span id="mqttPill" class="pill warn">MQTT: ...</span>
    <span id="bmsPill" class="pill no">BMS</span>
    <span id="invPill" class="pill no">İNV</span>
  </div>
</div>

<!-- 4'LÜ CANLI ENERJİ AKIŞ ÖZETİ -->
<div class="flow">
  <div class="fcard">
    <div class="lbl">🔋 Akü Doluluk (BMS)</div>
    <div class="val" style="color:var(--green)" id="soc">%--.-</div>
    <div class="det" id="bmsVW">--.- V • --.- A (--- W)</div>
  </div>
  <div class="fcard">
    <div class="lbl">☀️ 10×550W Solar (PV)</div>
    <div class="val" style="color:var(--amber)" id="pvW">--- W</div>
    <div class="det" id="pvVA">---.- V • --.- A</div>
  </div>
  <div class="fcard">
    <div class="lbl">🏠 Ev Tüketimi (Yük)</div>
    <div class="val" style="color:var(--blue)" id="loadW">--- W</div>
    <div class="det" id="loadDet">Yük: %-- • Çıkış: ---V</div>
  </div>
  <div class="fcard">
    <div class="lbl">🔌 Şebeke &amp; Mod</div>
    <div class="val" style="font-size:16px;margin-top:8px;color:var(--purple)" id="modeShort">Bekleniyor</div>
    <div class="det" id="gridDet">Şebeke: --- V / -- Hz</div>
  </div>
</div>

<!-- DALY 16S BMS DETAY -->
<div class="card">
  <h3><span>🔋 Daly 16S LiFePO4 (RS485 Kablolu)</span><span id="diffBadge" class="pill ok">Delta: -- mV</span></h3>
  <div class="soc-bar-bg"><div id="socBar" class="soc-bar-fill" style="width:0%"></div></div>
  <div class="row"><span>Kalan Kapasite &amp; Durum</span><strong id="bmsAh">---.- Ah / 280 Ah</strong></div>
  <div class="row"><span>🔄 Döngü Sayısı (Cycles)</span><strong id="bmsCyc" style="color:#38bdf8">-- Döngü</strong></div>
  <div class="row"><span>Max / Min Hücre</span><strong id="minMax">-.---V / -.---V</strong></div>
  <div class="row"><span>Sıcaklık / MOSFET</span><strong id="mos">-- °C | Şarj:- Deş:-</strong></div>
  <div class="row"><span>BMS Arıza / Alarm</span><strong id="bmsFault" style="color:#34d399">Arıza Yok</strong></div>
  <div class="row"><span>Akıllı BMS Durumu</span><strong id="smartMsg" style="color:#6ee7b7;text-align:right">---</strong></div>
  <div class="row"><span>Son İnverter / BMS Komutu</span><strong id="lastCmd" style="font-size:11px;color:#cbd5e1;text-align:right;max-width:65%">---</strong></div>
</div>

<!-- 16 HÜCRE MATRİSİ -->
<div class="card">
  <h3><span>📊 16 Hücre Canlı Voltajları</span><span class="pill ok" id="invTemp">🌡️ İnv: --°C</span></h3>
  <div class="cells" id="cellsGrid"></div>
</div>

<!-- DALY BMS TÜM İÇ AYARLARI & KORUMALARI (RS485) -->
<div class="card">
  <h3><span>🛡️ Daly BMS Tüm Ayarlar &amp; Koruma (RS485)</span><span id="bmsConnBadge" class="pill ok">RS485</span></h3>

  <div class="sgrid">
    <div class="scard"><div class="sl">🔄 Toplam Döngü</div><div class="sv" style="color:#38bdf8" id="bsCyc">--</div></div>
    <div class="scard"><div class="sl">🔋 Nominal Kap.</div><div class="sv" style="color:#10b981" id="bsCap">280 Ah</div></div>
    <div class="scard"><div class="sl">⚖️ 1A Balans</div><div class="sv" style="color:#fbbf24" id="bsBal">3.20V / 20mV</div></div>
  </div>

  <div class="sec-lbl" style="margin-top:4px"><span>⚡ BMS MOSFET Anahtarları (Dikkatli Kullanın)</span><small id="lblMosState">AÇIK / AÇIK</small></div>
  <div class="btns" style="margin-bottom:8px">
    <button id="btnChgMos" onclick="toggleBmsMos('CHGMOS', curBms.chgMos)">⚡ Şarj MOSFET</button>
    <button id="btnDisMos" onclick="toggleBmsMos('DISMOS', curBms.disMos)">🔌 Deşarj MOSFET</button>
  </div>

  <div class="row"><span>Hücre Yüksek Uyarı / Kesme (OVP)</span><strong id="bsOvp">3.550V / 3.600V</strong></div>
  <div class="row"><span>Hücre Düşük Uyarı / Kesme (UVP)</span><strong id="bsUvp">2.900V / 2.850V</strong></div>
  <div class="row"><span>Toplam Paket Kesme (OVP / UVP)</span><strong id="bsPack">57.6V / 45.6V</strong></div>
  <div class="row"><span>Şarj Aşırı Akım (Uyarı / Kesme)</span><strong id="bsCOcp">160A / 200A</strong></div>
  <div class="row"><span>Deşarj Aşırı Akım (Uyarı / Kesme)</span><strong id="bsDOcp">160A / 200A</strong></div>
  <div class="row"><span>Sıcaklık Koruması (Şarj / Deşarj)</span><strong id="bsTemp">0–55°C / 0–55°C</strong></div>
  <div class="row"><span>Uyku Zamanlayıcı / Sensör</span><strong id="bsSleep">60 dk • 1 NTC</strong></div>

  <div class="sec-lbl" style="margin-top:12px"><span>⚖️ 1A Aktif Balans Ayarını Değiştir (RS485)</span><small>Örn: 3.40V / 15mV</small></div>
  <div class="row">
    <span>Balans Başlama (V) &amp; Fark (mV)</span>
    <div class="v-edit">
      <input type="number" id="inBalV" value="3.40" step="0.01" style="width:64px">
      <input type="number" id="inBalMv" value="15" step="5" style="width:54px">
      <button class="sm blu" onclick="saveBmsBal()">Yaz</button>
    </div>
  </div>
</div>

<!-- İNVERTER TÜM AYARLARI (CANLI WATCHPOWER MENÜSÜ) -->
<div class="card">
  <h3><span>⚙️ İnverter Tüm Ayarları (WatchPower Canlı)</span><span id="invRatedBadge" class="pill info">5500W • 48V</span></h3>

  <div class="sec-lbl"><span>Prog 01 — Çıkış Önceliği (Akü / Şebeke)</span><small id="lblOutPrio">---</small></div>
  <div class="btns">
    <button id="btnPOP02" onclick="pubCmd('POP02')">SBU (Akü+Solar)</button>
    <button id="btnPOP01" onclick="pubCmd('POP01')">SOL (Önce Solar)</button>
    <button id="btnPOP00" onclick="pubCmd('POP00')">UTI (Şebeke)</button>
  </div>

  <div class="sec-lbl"><span>Prog 16 — Şarj Kaynağı Önceliği</span><small id="lblChgPrio">---</small></div>
  <div class="btns">
    <button id="btnPCP03" onclick="pubCmd('PCP03')">OSO (Sadece Güneş)</button>
    <button id="btnPCP01" onclick="pubCmd('PCP01')">CSO (Önce Güneş)</button>
    <button id="btnPCP02" onclick="pubCmd('PCP02')">SNU (Güneş+Şebeke)</button>
  </div>

  <div class="sec-lbl"><span>Prog 02 — Maks. Toplam Şarj Akımı (10×550W)</span><small id="lblMaxChg">-- A</small></div>
  <div class="btns">
    <button id="btnM10" onclick="pubCmd('MCHGC010')">10A</button>
    <button id="btnM20" onclick="pubCmd('MCHGC020')">20A</button>
    <button id="btnM30" onclick="pubCmd('MCHGC030')">30A</button>
    <button id="btnM40" onclick="pubCmd('MCHGC040')">40A</button>
    <button id="btnM60" onclick="pubCmd('MCHGC060')">60A</button>
    <button id="btnM80" onclick="pubCmd('MCHGC080')">80A</button>
    <button id="btnM90" onclick="pubCmd('MCHGC090')">90A</button>
    <button id="btnM100" onclick="pubCmd('MCHGC100')">100A</button>
    <button id="btnM110" onclick="pubCmd('MNCHGC0110')">110A</button>
  </div>

  <div class="sec-lbl"><span>Prog 11 — Şebekeden Şarj Akımı</span><small id="lblGridChg">-- A</small></div>
  <div class="btns">
    <button id="btnU2" onclick="pubCmd('MUCHGC002')">2A</button>
    <button id="btnU10" onclick="pubCmd('MUCHGC010')">10A</button>
    <button id="btnU20" onclick="pubCmd('MUCHGC020')">20A</button>
    <button id="btnU30" onclick="pubCmd('MUCHGC030')">30A</button>
    <button id="btnU40" onclick="pubCmd('MUCHGC040')">40A</button>
    <button id="btnU60" onclick="pubCmd('MUCHGC060')">60A</button>
  </div>

  <div class="sec-lbl"><span>Prog 05 — Akü Tipi</span><small id="lblBatType">---</small></div>
  <div class="btns">
    <button id="btnPBT00" onclick="pubCmd('PBT00')">AGM</button>
    <button id="btnPBT01" onclick="pubCmd('PBT01')">FLD (Sulu)</button>
    <button id="btnPBT02" onclick="pubCmd('PBT02')">USE (LiFePO4)</button>
  </div>

  <div class="sec-lbl"><span>Prog 03 — AC Giriş Voltaj Aralığı</span><small id="lblAcRange">---</small></div>
  <div class="btns">
    <button id="btnPGR00" onclick="pubCmd('PGR00')">APL (90-280V)</button>
    <button id="btnPGR01" onclick="pubCmd('PGR01')">UPS (170-280V)</button>
  </div>

  <div class="sec-lbl" style="margin-top:14px"><span>Voltaj Ayarları (Canlı Okunan &amp; Düzenle)</span><small>16S LiFePO4</small></div>
  <div class="row">
    <span>Prog 26 • Bulk Şarj (PCVV)</span>
    <div class="v-edit"><span class="v-cur" id="curBulk">--.-V</span><input type="number" id="inBulk" value="55.0" step="0.1"><button class="sm blu" onclick="sendV('PCVV','inBulk')">Yaz</button></div>
  </div>
  <div class="row">
    <span>Prog 27 • Float Tampon (PBFT)</span>
    <div class="v-edit"><span class="v-cur" id="curFloat">--.-V</span><input type="number" id="inFloat" value="53.6" step="0.1"><button class="sm blu" onclick="sendV('PBFT','inFloat')">Yaz</button></div>
  </div>
  <div class="row">
    <span>Prog 29 • Düşük Kesme (PSDV)</span>
    <div class="v-edit"><span class="v-cur" id="curCut">--.-V</span><input type="number" id="inCut" value="45.0" step="0.1"><button class="sm blu" onclick="sendV('PSDV','inCut')">Yaz</button></div>
  </div>
  <div class="row">
    <span>Prog 12 • Şebekeye Dönüş (PBCV)</span>
    <div class="v-edit"><span class="v-cur" id="curBGrid">--.-V</span><input type="number" id="inBGrid" value="46.0" step="0.5"><button class="sm blu" onclick="sendV('PBCV','inBGrid')">Yaz</button></div>
  </div>
  <div class="row">
    <span>Prog 13 • Aküye Dönüş (PBDV)</span>
    <div class="v-edit"><span class="v-cur" id="curBBat">--.-V</span><input type="number" id="inBBat" value="53.0" step="0.5"><button class="sm blu" onclick="sendV('PBDV','inBBat')">Yaz</button></div>
  </div>

  <div class="sec-lbl"><span>İnverter Sistem Anahtarları (QFLAG)</span></div>
  <div class="btns">
    <button id="btnBuz" onclick="toggleFlag('a', curSet.buz)">🔔 Buzzer</button>
    <button id="btnByp" onclick="toggleFlag('b', curSet.bypass)">🔄 Bypass</button>
    <button id="btnPwr" onclick="toggleFlag('j', curSet.pwrsav)">🌙 Tasarruf</button>
    <button id="btnLcd" onclick="toggleFlag('x', curSet.lcd)">💡 LCD Işık</button>
  </div>

  <div class="btns" style="margin-top:12px">
    <button class="pur" onclick="applyLiFePO4()">⚡ Tek Tuşla 16S LiFePO4 Boost Profilini Uygula (56.8 / 56.0 / 45 / 46V)</button>
  </div>
</div>

<!-- AKILLI OTOMASYON AYARLARI (BOOST & CUT + TERMAL BYPASS) -->
<div class="card">
  <h3><span>🤖 Boost &amp; Cut + Termal Koruma Otomasyonu</span><span id="capBadge" class="pill ok">Üst Sınır: %90</span></h3>

  <div class="sec-lbl" style="margin-top:2px"><span>🛑 Maksimum Şarj Sınırı (Akü Bu Yüzdeyi Geçmez!)</span><small id="lblMaxSoc">%90</small></div>
  <div class="btns" style="margin-bottom:10px">
    <button id="btnCap80" onclick="setMaxSoc(80)">%80</button>
    <button id="btnCap90" onclick="setMaxSoc(90)">%90 (Ömür)</button>
    <button id="btnCap92" onclick="setMaxSoc(92)">%92</button>
    <button id="btnCap95" onclick="setMaxSoc(95)">%95</button>
    <button id="btnCap100" onclick="setMaxSoc(100)">%100 (Tam)</button>
  </div>

  <div class="sec-lbl" style="margin-top:6px"><span>🌡️ İnverter Aşırı Isınma Koruması (PI30 Termal Kısma + Bypass)</span><span id="thermBadge" class="pill ok">Normal</span></div>

  <div class="plan-box">
    <div><b>⚡ 1. Aşama — Hızlı Boost Şarj (&lt;%90 &amp; Hücre &lt;3.45V):</b> İnverter <b>56.8V Bulk / 56.0V Float</b> + <span class="w">100A</span> ile kablo düşümünü ve Voltronic erken-Float hatasını tamamen aşar, paneller tam hız basar!</div>
    <div style="margin-top:4px"><b>⚖️ 2. Aşama — Otomatik Voltaj Düşürme (Hücre &ge;3.45V):</b> Hücreler tepeye yaklaşınca voltajı otomatik <b>55.2V / 54.0V</b>'a, akımı <span class="w">40A</span> (&ge;3.49V &rarr; <span class="w">10A</span>)'e düşürür.</div>
    <div style="margin-top:4px"><b>🛑 3. Aşama — %90 Kesin Durdurma (veya Hücre &ge;3.51V):</b> Anında <b>52.8V / 0A</b> keser (paneller evi besler); <b>%87</b>'de tekrar tam hız açar. <b>%20</b>'de Şebeke (UTI) &rarr; <b>%30</b>'da Akü (SBU).</div>
    <div style="margin-top:4px"><b>🌡️ PI30 Termal Koruma:</b> İnverter <b>&ge;65°C</b> &rarr; <span class="w">40A</span>, <b>&ge;72°C</b> &rarr; <span class="w">10A</span>, <b>&ge;78°C</b> &rarr; <b>ACİL ŞEBEKE BYPASS (POP00 + 10A)</b>! <b>&le;65°C</b> soğuyunca otomatik SBU'ya döner.</div>
  </div>

  <div class="row"><span>Otomatik BMS &amp; Termal Yönetim</span><input class="switch" type="checkbox" id="cAuto" checked></div>
  <div class="row"><span>🛑 Maksimum Şarj Sınırı (% Geçmesin)</span><input type="number" id="cMaxS" value="90" min="50" max="100" step="1"></div>
  <div class="row"><span>⚡ Boost Bulk Voltajı (PCVV) (V)</span><input type="number" id="cBulkV" value="56.8" min="52.0" max="57.2" step="0.1"></div>
  <div class="row"><span>⚡ Boost Float Voltajı (PBFT) (V)</span><input type="number" id="cFloatV" value="56.0" min="51.0" max="56.8" step="0.1"></div>
  <div class="row"><span>Şebekeye Geçiş (% SoC Altı)</span><input type="number" id="cSocG" value="20" step="1"></div>
  <div class="row"><span>Aküye Dönüş (% SoC Üstü)</span><input type="number" id="cSocB" value="30" step="1"></div>
  <div class="row"><span>Acil Şebeke Şarjı Aç (% Altı)</span><input type="number" id="cChgOn" value="12" step="1"></div>
  <div class="row"><span>Şebeke Şarjını Kapat (% Üstü)</span><input type="number" id="cChgOff" value="20" step="1"></div>
  <div class="row"><span>Hızlı Boost Şarj Akımı (A)</span><input type="number" id="cMaxA" value="100" step="10"></div>
  <div class="row"><span>Yumuşatma / Termal-1 Hızı (A)</span><input type="number" id="cMidA" value="40" step="10"></div>
  <div class="row"><span>Balans / Termal-2 Hızı (A)</span><input type="number" id="cBalA" value="10" step="10"></div>
  <div class="row"><span>Voltaj/Akım Düşürme Hücre Eşiği (V)</span><input type="number" id="cTap1" value="3.45" step="0.01"></div>
  <div class="row"><span>1A Balans Hücre Eşiği (V)</span><input type="number" id="cTap2" value="3.49" step="0.01"></div>
  <div class="row"><span>🌡️ İnv. Kademe-1 Kısma → 40A (°C)</span><input type="number" id="cTWarn" value="65" min="45" max="78" step="1"></div>
  <div class="row"><span>🔥 İnv. Kademe-2 Kısma → 10A (°C)</span><input type="number" id="cTHigh" value="72" min="50" max="84" step="1"></div>
  <div class="row"><span>🚨 İnv. Acil Şebeke Bypass (°C)</span><input type="number" id="cTByp" value="78" min="55" max="90" step="1"></div>

  <div class="btns" style="margin-top:12px">
    <button class="act" onclick="saveCfg()">💾 Boost &amp; Termal Kuralları ESP32'ye Kaydet</button>
  </div>
</div>

<!-- GÜNLÜK & AYLIK ENERJİ İSTATİSTİKLERİ + GRAFİKLER -->
<div class="card">
  <h3><span>📊 Enerji İstatistikleri &amp; Aylık Grafik</span><span id="statDate" class="pill info">--.--.----</span></h3>

  <div class="btns" style="margin-bottom:10px">
    <button id="tabMonth" class="act" onclick="setChartTab('month')">📅 Bu Ay (30 Gün)</button>
    <button id="tabToday" onclick="setChartTab('today')">☀️ Bugün (24 Saat)</button>
  </div>

  <div class="sgrid">
    <div class="scard"><div class="sl">☀️ Panel Üretimi</div><div class="sv" style="color:#fbbf24" id="stPv">0.00 kWh</div></div>
    <div class="scard"><div class="sl">🏠 Ev Tüketimi</div><div class="sv" style="color:#38bdf8" id="stLoad">0.00 kWh</div></div>
    <div class="scard"><div class="sl">🔋 Akü Dolumu</div><div class="sv" style="color:#10b981" id="stChg">0.00 kWh</div></div>
    <div class="scard"><div class="sl">🔋 Akü Kullanımı</div><div class="sv" style="color:#2dd4bf" id="stDis">0.00 kWh</div></div>
    <div class="scard"><div class="sl">🔌 Şebeke Çıkışı</div><div class="sv" style="color:#fb923c" id="stGridOut">0.00 kWh</div></div>
    <div class="scard"><div class="sl">⚡ Şebeke Şarjı</div><div class="sv" style="color:#c084fc" id="stGridChg">0.00 kWh</div></div>
  </div>

  <div class="row" style="padding:4px 0 8px 0">
    <span>☀️ Güneşten Karşılama Oranı</span>
    <strong id="stSolarPct" style="color:#34d399">%100</strong>
  </div>

  <div class="legend">
    <span class="lg"><i class="dot" style="background:#fbbf24"></i>Panel Dolumu</span>
    <span class="lg"><i class="dot" style="background:#38bdf8"></i>Ev Kullanımı</span>
    <span class="lg"><i class="dot" style="background:#fb923c"></i>Şebeke Çıkışı</span>
    <span class="lg"><i class="dot" style="background:#c084fc"></i>Şebeke Şarjı</span>
  </div>

  <div class="chart-wrap">
    <canvas id="energyChart" height="210"></canvas>
  </div>

  <div class="btns" style="margin-top:10px">
    <button onclick="resetStats()" style="font-size:11.5px;color:#fca5a5;border-color:#7f1d1d">🗑️ Aylık Sayaçları Sıfırla</button>
  </div>
</div>

<div id="toast" class="toast">Komut gönderildi</div>

<script>
const MQTT_URL    = 'wss://broker.emqx.io:8084/mqtt';
const MQTT_PREFIX = (window.MQTT_PREFIX || 'dalyinv/BENZERSIZ_CIHAZ_ID');
const T_STATUS    = MQTT_PREFIX + '/status';
const T_STATS     = MQTT_PREFIX + '/stats';
const T_CMD       = MQTT_PREFIX + '/cmd';
const T_CFG       = MQTT_PREFIX + '/cfg';

const IS_LOCAL_ESP = (location.hostname === '192.168.1.140' || location.hostname === 'akilli-inverter.local');

let client = null;
let firstCfg = true;
let firstSet = true;
let firstBmsSet = true;
let curSet = {};
let curBms = {};
let curStats = null;
let chartTab = 'month';

function toast(msg){
  const el = document.getElementById('toast');
  el.innerText = msg;
  el.classList.add('show');
  setTimeout(()=>el.classList.remove('show'), 2600);
}

function setActive(ids, activeId){
  ids.forEach(id => {
    const el = document.getElementById(id);
    if(el) el.className = (id === activeId) ? 'act' : '';
  });
}

function connectMqtt(){
  if(IS_LOCAL_ESP){
    document.getElementById('mqttPill').className = 'pill ok';
    document.getElementById('mqttPill').innerText = 'YEREL WI-FI';
    pollLocal();
    pollLocalStats();
    setInterval(pollLocal, 2500);
    setInterval(pollLocalStats, 12000);
    return;
  }
  const cid = 'APK_' + Math.random().toString(16).slice(2, 10);
  client = mqtt.connect(MQTT_URL, {
    clientId: cid,
    clean: true,
    reconnectPeriod: 3000,
    connectTimeout: 10000
  });
  client.on('connect', () => {
    document.getElementById('mqttPill').className = 'pill ok';
    document.getElementById('mqttPill').innerText = 'MQTT: CANLI';
    client.subscribe(T_STATUS);
    client.subscribe(T_STATS);
  });
  client.on('reconnect', () => {
    document.getElementById('mqttPill').className = 'pill warn';
    document.getElementById('mqttPill').innerText = 'BAĞLANIYOR';
  });
  client.on('error', () => {
    document.getElementById('mqttPill').className = 'pill no';
    document.getElementById('mqttPill').innerText = 'MQTT: HATA';
  });
  client.on('message', (topic, message) => {
    try{
      if(topic === T_STATUS) renderData(JSON.parse(message.toString()));
      else if(topic === T_STATS){ curStats = JSON.parse(message.toString()); renderStats(); }
    }catch(e){}
  });
}

async function pollLocal(){
  try{
    const r = await fetch('/api/status');
    renderData(await r.json());
  }catch(e){}
}

async function pollLocalStats(){
  try{
    const r = await fetch('/api/stats');
    curStats = await r.json();
    renderStats();
  }catch(e){}
}

function renderData(d){
  const b = d.bms || {}, bs = d.bmsSet || {}, i = d.inv || {}, s = d.set || {}, c = d.cfg || {};
  curSet = s;
  curBms = b;
  const nowStr = new Date().toLocaleTimeString('tr-TR');
  document.getElementById('subInfo').innerText = '10×550W • 280Ah LiFePO4 • Son: ' + nowStr;

  document.getElementById('bmsPill').className = 'pill ' + (b.ok?'ok':'no');
  document.getElementById('bmsPill').innerText = 'BMS: ' + (b.ok?'RS485':'YOK');
  document.getElementById('invPill').className = 'pill ' + (i.ok?'ok':'no');
  document.getElementById('invPill').innerText = 'İNV: ' + (i.ok?'OK':'YOK');

  const soc = Number(b.soc||0);
  const stNames = ['Boşta', 'Şarjda ⚡', 'Deşarjda 🔋'];
  document.getElementById('soc').innerText = '%' + soc.toFixed(1);
  document.getElementById('socBar').style.width = Math.max(0, Math.min(100, soc)) + '%';
  document.getElementById('bmsVW').innerText = Number(b.v||0).toFixed(2) + 'V • ' + (b.a>=0?'+':'') + Number(b.a||0).toFixed(1) + 'A (' + (b.w||0) + 'W)';
  document.getElementById('bmsAh').innerText = Number(b.ah||0).toFixed(1) + ' Ah / ' + Number(bs.ratedAh||280).toFixed(0) + ' Ah • ' + (stNames[b.st]||'-');
  document.getElementById('bmsCyc').innerText = (b.cyc||0) + ' Döngü';
  document.getElementById('minMax').innerText = 'H'+(b.maxN||'-')+': '+Number(b.maxV||0).toFixed(3)+'V / H'+(b.minN||'-')+': '+Number(b.minV||0).toFixed(3)+'V';
  document.getElementById('diffBadge').innerText = 'Delta: ' + (b.diff||0) + ' mV';
  document.getElementById('diffBadge').className = 'pill ' + ((b.diff||0)>40?'warn':'ok');
  document.getElementById('mos').innerText = (b.tMax||0) + ' °C | Şarj:' + (b.chgMos?'AÇIK':'KAPALI') + ' Deş:' + (b.disMos?'AÇIK':'KAPALI');
  const fEl = document.getElementById('bmsFault');
  fEl.innerText = b.fault || 'Arıza Yok';
  fEl.style.color = (b.fault && b.fault !== 'Arıza Yok') ? '#f87171' : '#34d399';

  document.getElementById('pvW').innerText = (i.pvW||0) + ' W';
  document.getElementById('pvVA').innerText = Number(i.pvV||0).toFixed(1) + ' V • ' + Number(i.pvA||0).toFixed(1) + ' A';
  document.getElementById('loadW').innerText = (i.loadW||0) + ' W';
  document.getElementById('loadDet').innerText = 'Yük: %' + (i.loadPct||0) + ' • Çıkış: ' + Number(i.outV||0).toFixed(0) + 'V';
  document.getElementById('modeShort').innerText = i.mode || 'Bilinmiyor';
  document.getElementById('gridDet').innerText = 'Şebeke: ' + Number(i.gridV||0).toFixed(0) + 'V / ' + Number(i.gridHz||0).toFixed(1) + 'Hz';
  const tInv = Number(i.temp||0);
  const tLvl = Number(i.thermLvl||0);
  const invTempEl = document.getElementById('invTemp');
  const thEl = document.getElementById('thermBadge');
  if(tLvl >= 3 || i.thermByp){
    invTempEl.className = 'pill no';
    invTempEl.innerText = '🚨 İnv: ' + tInv + '°C • ACİL BYPASS';
    thEl.className = 'pill no';
    thEl.innerText = '🚨 ' + tInv + '°C • ACİL ŞEBEKE BYPASS';
  } else if(tLvl === 2){
    invTempEl.className = 'pill no';
    invTempEl.innerText = '🔥 İnv: ' + tInv + '°C • 10A';
    thEl.className = 'pill no';
    thEl.innerText = '🔥 ' + tInv + '°C • Kısma-2 (10A)';
  } else if(tLvl === 1){
    invTempEl.className = 'pill warn';
    invTempEl.innerText = '⚠️ İnv: ' + tInv + '°C • 40A';
    thEl.className = 'pill warn';
    thEl.innerText = '⚠️ ' + tInv + '°C • Kısma-1 (40A)';
  } else {
    invTempEl.className = 'pill ok';
    invTempEl.innerText = '🌡️ İnv: ' + tInv + '°C';
    thEl.className = 'pill ok';
    thEl.innerText = '🌡️ ' + tInv + '°C • Normal';
  }
  document.getElementById('smartMsg').innerText = i.smart || 'Aktif';
  document.getElementById('lastCmd').innerText = i.lastCmd || '---';

  // 16 Hücre (+ Canlı 1A Aktif Balans İkonu)
  if(Array.isArray(b.cells) && b.cells.length === 16){
    const bMask = Number(b.balMask||0);
    let html = '';
    for(let k=0;k<16;k++){
      const v = Number(b.cells[k]||0);
      const pct = Math.max(0, Math.min(100, ((v - 2.80) / (3.65 - 2.80)) * 100));
      const isBal = ((bMask >> k) & 1) === 1;
      let cls = 'cell';
      if(k+1 === b.maxN) cls += ' hi';
      else if(k+1 === b.minN) cls += ' lo';
      html += `<div class="${cls}"><div class="cn">H${k+1}${isBal?' ⚖️':''}</div><div class="cv">${v.toFixed(3)}V</div><div class="cb"><div class="cbf" style="width:${pct.toFixed(0)}%"></div></div></div>`;
    }
    document.getElementById('cellsGrid').innerHTML = html;
  }

  // Daly BMS İç Ayarlar & Koruma Kartı
  document.getElementById('bmsConnBadge').innerText = b.ip || 'RS485';
  document.getElementById('bsCyc').innerText = (b.cyc||0) + ' Döngü';
  document.getElementById('lblMosState').innerText = 'Şarj:' + (b.chgMos?'AÇIK':'KAPALI') + ' • Deş:' + (b.disMos?'AÇIK':'KAPALI');
  document.getElementById('btnChgMos').className = b.chgMos ? 'act' : '';
  document.getElementById('btnDisMos').className = b.disMos ? 'act' : '';
  if(bs && bs.ok){
    document.getElementById('bsCap').innerText = Number(bs.ratedAh).toFixed(0) + ' Ah';
    document.getElementById('bsBal').innerText = Number(bs.balV).toFixed(2) + 'V / ' + bs.balMv + 'mV';
    document.getElementById('bsOvp').innerText = Number(bs.ovp1).toFixed(3) + 'V / ' + Number(bs.ovp2).toFixed(3) + 'V';
    document.getElementById('bsUvp').innerText = Number(bs.uvp1).toFixed(3) + 'V / ' + Number(bs.uvp2).toFixed(3) + 'V';
    document.getElementById('bsPack').innerText = Number(bs.pOvp).toFixed(1) + 'V / ' + Number(bs.pUvp).toFixed(1) + 'V';
    document.getElementById('bsCOcp').innerText = bs.cOcp1 + 'A / ' + bs.cOcp2 + 'A';
    document.getElementById('bsDOcp').innerText = bs.dOcp1 + 'A / ' + bs.dOcp2 + 'A';
    document.getElementById('bsTemp').innerText = bs.cTLo + '–' + bs.cTHi + '°C / ' + bs.dTLo + '–' + bs.dTHi + '°C';
    document.getElementById('bsSleep').innerText = Math.round((bs.sleep||3600)/60) + ' dk • ' + (b.ntc||1) + ' NTC';
    if(firstBmsSet){
      document.getElementById('inBalV').value  = Number(bs.balV).toFixed(2);
      document.getElementById('inBalMv').value = bs.balMv;
      firstBmsSet = false;
    }
  }

  // İnverter Tüm Ayarları (QPIRI + QFLAG)
  if(s && s.ok){
    document.getElementById('invRatedBadge').innerText = (s.ratedW||5500) + 'W • 48V';
    const outNames = ['UTI (Şebeke)', 'SOL (Önce Solar)', 'SBU (Akü+Solar)'];
    const chgNames = ['UTI', 'CSO (Önce Güneş)', 'SNU (Güneş+Şebeke)', 'OSO (Sadece Güneş)'];
    const batNames = ['AGM', 'FLD (Sulu)', 'USE (LiFePO4)'];
    const acNames  = ['APL (90-280V)', 'UPS (170-280V)'];

    document.getElementById('lblOutPrio').innerText = 'Aktif: ' + (outNames[s.outPrio] || s.outPrio);
    setActive(['btnPOP00','btnPOP01','btnPOP02'], 'btnPOP0' + s.outPrio);

    document.getElementById('lblChgPrio').innerText = 'Aktif: ' + (chgNames[s.chgPrio] || s.chgPrio);
    setActive(['btnPCP01','btnPCP02','btnPCP03'], 'btnPCP0' + s.chgPrio);

    document.getElementById('lblMaxChg').innerText = 'Aktif: ' + s.maxChgA + ' A';
    setActive(['btnM10','btnM20','btnM30','btnM40','btnM60','btnM80','btnM90','btnM100','btnM110'], 'btnM' + s.maxChgA);

    document.getElementById('lblGridChg').innerText = 'Aktif: ' + s.gridChgA + ' A';
    setActive(['btnU2','btnU10','btnU20','btnU30','btnU40','btnU60'], 'btnU' + s.gridChgA);

    document.getElementById('lblBatType').innerText = 'Aktif: ' + (batNames[s.battType] || s.battType);
    setActive(['btnPBT00','btnPBT01','btnPBT02'], 'btnPBT0' + s.battType);

    document.getElementById('lblAcRange').innerText = 'Aktif: ' + (acNames[s.acRange] || s.acRange);
    setActive(['btnPGR00','btnPGR01'], 'btnPGR0' + s.acRange);

    document.getElementById('curBulk').innerText  = Number(s.bulkV).toFixed(1) + 'V';
    document.getElementById('curFloat').innerText = Number(s.floatV).toFixed(1) + 'V';
    document.getElementById('curCut').innerText   = Number(s.cutOffV).toFixed(1) + 'V';
    document.getElementById('curBGrid').innerText = Number(s.backGridV).toFixed(1) + 'V';
    document.getElementById('curBBat').innerText  = Number(s.backBatV).toFixed(1) + 'V';

    if(firstSet){
      document.getElementById('inBulk').value  = Number(s.bulkV).toFixed(1);
      document.getElementById('inFloat').value = Number(s.floatV).toFixed(1);
      document.getElementById('inCut').value   = Number(s.cutOffV).toFixed(1);
      document.getElementById('inBGrid').value = Number(s.backGridV).toFixed(1);
      document.getElementById('inBBat').value  = Number(s.backBatV).toFixed(1);
      firstSet = false;
    }

    document.getElementById('btnBuz').className = s.buz    ? 'act' : '';
    document.getElementById('btnByp').className = s.bypass ? 'act' : '';
    document.getElementById('btnPwr').className = s.pwrsav ? 'act' : '';
    document.getElementById('btnLcd').className = s.lcd    ? 'act' : '';
  }

  // Üst Şarj Sınırı Canlı Rozeti ve Butonları
  if(c.maxSoc !== undefined){
    const ms = Math.round(Number(c.maxSoc));
    document.getElementById('lblMaxSoc').innerText = (ms >= 100) ? 'Sınırsız (%100)' : ('Maks %' + ms);
    setActive(['btnCap80','btnCap90','btnCap92','btnCap95','btnCap100'], 'btnCap' + ms);
    const cb = document.getElementById('capBadge');
    if(c.socCap){
      cb.className = 'pill warn';
      cb.innerText = '🛑 SINIR DEVREDE (%' + ms + ' / 0A)';
    } else {
      cb.className = 'pill ok';
      cb.innerText = (ms >= 100) ? 'Tam Dolum (%100)' : ('Üst Sınır: %' + ms);
    }
  }

  // Otomasyon ayarları kutularını doldur
  if(firstCfg && c.socGrid !== undefined){
    document.getElementById('cAuto').checked = !!c.auto;
    if(c.maxSoc !== undefined) document.getElementById('cMaxS').value = Math.round(Number(c.maxSoc));
    if(c.bulkV  !== undefined) document.getElementById('cBulkV').value  = Number(c.bulkV).toFixed(1);
    if(c.floatV !== undefined) document.getElementById('cFloatV').value = Number(c.floatV).toFixed(1);
    document.getElementById('cSocG').value   = c.socGrid;
    document.getElementById('cSocB').value   = c.socBat;
    if(c.socChgOn  !== undefined) document.getElementById('cChgOn').value  = c.socChgOn;
    if(c.socChgOff !== undefined) document.getElementById('cChgOff').value = c.socChgOff;
    document.getElementById('cMaxA').value   = c.maxA;
    if(c.midA !== undefined) document.getElementById('cMidA').value = c.midA;
    if(c.balA !== undefined) document.getElementById('cBalA').value = c.balA;
    document.getElementById('cTap1').value   = c.taper1;
    document.getElementById('cTap2').value   = c.taper2;
    if(c.tWarn !== undefined) document.getElementById('cTWarn').value = c.tWarn;
    if(c.tHigh !== undefined) document.getElementById('cTHigh').value = c.tHigh;
    if(c.tByp  !== undefined) document.getElementById('cTByp').value  = c.tByp;
    firstCfg = false;
  }
}

async function pubCmd(cmdStr){
  if(IS_LOCAL_ESP){
    await fetch('/api/cmd?c=' + encodeURIComponent(cmdStr));
    toast('Komut gönderildi: ' + cmdStr);
    setTimeout(pollLocal, 600);
    return;
  }
  if(!client || !client.connected){ toast('MQTT bağlantısı bekleniyor...'); return; }
  client.publish(T_CMD, cmdStr);
  toast('Komut gönderildi: ' + cmdStr);
}

function sendV(prefix, inputId){
  const val = Number(document.getElementById(inputId).value).toFixed(1);
  pubCmd(prefix + val);
}

function toggleFlag(ch, isEnabled){
  pubCmd((isEnabled ? 'PD' : 'PE') + ch);
}

function toggleBmsMos(which, isOn){
  const label = (which === 'CHGMOS') ? 'Şarj MOSFET' : 'Deşarj MOSFET';
  const next = isOn ? 0 : 1;
  if(!confirm(`DİKKAT: Daly BMS ${label} ${next?'AÇILSIN':'KAPATILSIN'} mı?`)) return;
  pubCmd(`BMS:${which}:${next}`);
}

function saveBmsBal(){
  const mv = Math.round(Number(document.getElementById('inBalV').value) * 1000);
  const diff = Math.round(Number(document.getElementById('inBalMv').value));
  if(!confirm(`Daly BMS 1A Balans ayarı ${mv}mV başlama / ${diff}mV fark olarak yazılsın mı?`)) return;
  pubCmd(`BMS:BAL:${mv}:${diff}`);
}

async function applyLiFePO4(){
  if(!confirm('İnvertere 16S LiFePO4 Boost & Cut (Bulk: 56.8V, Float: 56.0V, Kesme: 45.0V, Şebeke: 46.0V, Akü: 53.0V) profili yazılsın mı?')) return;
  if(IS_LOCAL_ESP){
    await fetch('/api/apply_lifepo4');
    toast('16S LiFePO4 Boost Profili Yazıldı!');
    setTimeout(pollLocal, 800);
    return;
  }
  pubCmd('APPLY_LIFEPO4');
}

function setMaxSoc(val){
  document.getElementById('cMaxS').value = val;
  saveCfg();
}

async function saveCfg(){
  const qs = `auto=${document.getElementById('cAuto').checked?1:0}&maxS=${document.getElementById('cMaxS').value}&bulkV=${document.getElementById('cBulkV').value}&floatV=${document.getElementById('cFloatV').value}&socG=${document.getElementById('cSocG').value}&socB=${document.getElementById('cSocB').value}&chgOn=${document.getElementById('cChgOn').value}&chgOff=${document.getElementById('cChgOff').value}&maxA=${document.getElementById('cMaxA').value}&midA=${document.getElementById('cMidA').value}&balA=${document.getElementById('cBalA').value}&tap1=${document.getElementById('cTap1').value}&tap2=${document.getElementById('cTap2').value}&tWarn=${document.getElementById('cTWarn').value}&tHigh=${document.getElementById('cTHigh').value}&tByp=${document.getElementById('cTByp').value}`;
  if(IS_LOCAL_ESP){
    await fetch('/api/cfg?' + qs);
    toast('Otomasyon ve termal kurallar kaydedildi!');
    pollLocal();
    return;
  }
  if(!client || !client.connected){ toast('MQTT bağlantısı bekleniyor...'); return; }
  client.publish(T_CFG, qs);
  toast('Otomasyon ve termal kurallar ESP32\'ye kaydedildi!');
}

function setChartTab(tab){
  chartTab = tab;
  document.getElementById('tabMonth').className = (tab === 'month') ? 'act' : '';
  document.getElementById('tabToday').className = (tab === 'today') ? 'act' : '';
  renderStats();
}

async function resetStats(){
  if(!confirm('Tüm günlük ve aylık (30 günlük) kWh enerji sayaçları sıfırlansın mı?')) return;
  if(IS_LOCAL_ESP){
    await fetch('/api/stats_reset');
    pollLocalStats();
    toast('Sayaçlar sıfırlandı!');
    return;
  }
  pubCmd('RESET_STATS');
}

function renderStats(){
  if(!curStats) return;
  document.getElementById('statDate').innerText = curStats.dt || '--.--.----';
  const src = (chartTab === 'month') ? (curStats.month || {}) : (curStats.today || {});
  const dec = (chartTab === 'month') ? 2 : 3;
  const pv = Number(src.pv||0), ld = Number(src.load||0), ch = Number(src.chg||0),
        ds = Number(src.dis||0), go = Number(src.gridOut||0), gc = Number(src.gridChg||0);

  document.getElementById('stPv').innerText      = pv.toFixed(dec) + ' kWh';
  document.getElementById('stLoad').innerText    = ld.toFixed(dec) + ' kWh';
  document.getElementById('stChg').innerText     = ch.toFixed(dec) + ' kWh';
  document.getElementById('stDis').innerText     = ds.toFixed(dec) + ' kWh';
  document.getElementById('stGridOut').innerText = go.toFixed(dec) + ' kWh';
  document.getElementById('stGridChg').innerText = gc.toFixed(dec) + ' kWh';

  const totalNeed = ld + gc;
  const solarShare = totalNeed > 0.001 ? Math.max(0, Math.min(100, ((totalNeed - (go + gc)) / totalNeed) * 100)) : 100;
  document.getElementById('stSolarPct').innerText = '%' + solarShare.toFixed(1) + ' Güneş / Akü';

  drawChart();
}

function drawChart(){
  const cv = document.getElementById('energyChart');
  if(!cv || !curStats) return;
  const dpr = window.devicePixelRatio || 1;
  const W = Math.max(cv.parentElement.clientWidth - 16, 300);
  const H = 210;
  cv.width = W * dpr;
  cv.height = H * dpr;
  cv.style.width = W + 'px';
  cv.style.height = H + 'px';
  const ctx = cv.getContext('2d');
  ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, W, H);

  const padL = 36, padR = 10, padT = 16, padB = 26;
  const cW = W - padL - padR, cH = H - padT - padB;

  let labels = [], s1 = [], s2 = [], s3 = [], s4 = [];
  if(chartTab === 'month'){
    const days = curStats.days || [];
    days.forEach(d => {
      labels.push(d[0]);
      s1.push(Number(d[1]||0)); // PV
      s2.push(Number(d[2]||0)); // Load
      s3.push(Number(d[4]||0)); // GridOut
      s4.push(Number(d[5]||0)); // GridChg
    });
  } else {
    const hp = curStats.hPv || [], hl = curStats.hLoad || [], hg = curStats.hGrid || [];
    for(let h=0; h<24; h++){
      labels.push((h<10?'0':'')+h);
      s1.push(Number(hp[h]||0));
      s2.push(Number(hl[h]||0));
      s3.push(Number(hg[h]||0));
      s4.push(0);
    }
  }

  let maxV = 0.2;
  for(let i=0;i<labels.length;i++) maxV = Math.max(maxV, s1[i], s2[i], s3[i], s4[i]);
  maxV *= 1.15;

  // Yatay kılavuz çizgileri
  ctx.strokeStyle = '#24324d';
  ctx.fillStyle = '#94a3b8';
  ctx.font = '10px system-ui, sans-serif';
  ctx.lineWidth = 1;
  for(let g=0; g<=4; g++){
    const y = padT + cH - (cH * g / 4);
    ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(W - padR, y); ctx.stroke();
    const val = (maxV * g / 4);
    ctx.fillText(val >= 10 ? val.toFixed(0) : val.toFixed(2), 2, y + 3);
  }

  const n = Math.max(labels.length, 1);
  const slotW = cW / n;
  const cols = ['#fbbf24', '#38bdf8', '#fb923c', '#c084fc'];
  const series = [s1, s2, s3, s4];
  const barCnt = (chartTab === 'month') ? 4 : 3;
  const barW = Math.max(2, Math.min(10, (slotW * 0.78) / barCnt));

  for(let i=0; i<labels.length; i++){
    const x0 = padL + i * slotW + (slotW - barW * barCnt) / 2;
    for(let b=0; b<barCnt; b++){
      const v = series[b][i] || 0;
      if(v <= 0) continue;
      const bh = Math.max(2, (v / maxV) * cH);
      ctx.fillStyle = cols[b];
      ctx.fillRect(x0 + b * barW, padT + cH - bh, barW - 0.8, bh);
    }
    const step = (labels.length > 14) ? Math.ceil(labels.length / 8) : 1;
    if(i % step === 0 || i === labels.length - 1){
      ctx.fillStyle = '#94a3b8';
      ctx.fillText(labels[i], padL + i * slotW + 2, H - 8);
    }
  }
}

window.addEventListener('resize', drawChart);
connectMqtt();
</script>
</body>
</html>

)HTMLEND";

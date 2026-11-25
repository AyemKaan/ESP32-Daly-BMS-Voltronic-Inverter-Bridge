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
    --bg:#080e1a;--card:#111a2b;--card2:#0c1424;--border:#1f2d47;
    --green:#10b981;--blue:#38bdf8;--amber:#f59e0b;--red:#ef4444;--purple:#a78bfa;
    --txt:#f8fafc;--sub:#94a3b8
  }
  *{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent;font-family:system-ui,-apple-system,'Segoe UI',Roboto,sans-serif}
  body{background:var(--bg);color:var(--txt);padding:10px 10px 64px 10px;max-width:760px;margin:0 auto;user-select:none}

  /* Üst Başlık & Ana Sekme Çubuğu */
  .hdr{display:flex;justify-content:space-between;align-items:center;gap:8px;background:linear-gradient(135deg,#152036,#0d1526);padding:12px 14px;border-radius:16px;border:1px solid var(--border);margin-bottom:10px}
  .title{font-size:15.5px;font-weight:800;letter-spacing:-.2px}
  .sub{font-size:11px;color:var(--sub);margin-top:2px}
  .pills{display:flex;gap:5px;flex-wrap:wrap;justify-content:flex-end}
  .pill{padding:3.5px 8px;border-radius:99px;font-size:10.5px;font-weight:700;display:inline-flex;align-items:center;gap:4px;white-space:nowrap}
  .ok{background:#064e3b;color:#6ee7b7;border:1px solid #05966966}
  .warn{background:#78350f;color:#fcd34d;border:1px solid #d9770666}
  .no{background:#7f1d1d;color:#fca5a5;border:1px solid #dc262666}
  .info{background:#1e3a5f;color:#7dd3fc;border:1px solid #0284c766}

  .main-nav{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:10px;background:var(--card2);padding:5px;border-radius:14px;border:1px solid var(--border)}
  .nav-btn{padding:10px 8px;border-radius:10px;border:1px solid transparent;background:transparent;color:var(--sub);font-weight:800;font-size:13px;cursor:pointer;transition:.15s}
  .nav-btn.act{background:#162540;color:#fff;border-color:#2d466e;box-shadow:0 4px 14px rgba(0,0,0,.3)}

  .view{display:none}
  .view.show{display:block}

  /* Kompakt Tek Ekran Canlı Sensör Kartları */
  .flow{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:10px}
  .fcard{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:12px;cursor:pointer;transition:border-color .15s,transform .1s;position:relative}
  .fcard:active{transform:scale(.985)}
  .fcard:hover{border-color:#334c75}
  .fcard .top{display:flex;justify-content:space-between;align-items:center;gap:4px}
  .fcard .lbl{font-size:10.5px;color:var(--sub);font-weight:700;text-transform:uppercase;letter-spacing:.3px}
  .fcard .hint{font-size:10px;color:#64748b;font-weight:600}
  .fcard .val{font-size:22px;font-weight:800;margin-top:4px;line-height:1.15;font-variant-numeric:tabular-nums}
  .fcard .det{font-size:11.5px;color:#cbd5e1;margin-top:4px;font-variant-numeric:tabular-nums}
  .fcard .det2{font-size:10.5px;color:var(--sub);margin-top:2px;font-variant-numeric:tabular-nums}

  .soc-bar-bg{height:7px;background:#070c17;border-radius:99px;overflow:hidden;border:1px solid var(--border);margin-top:6px}
  .soc-bar-fill{height:100%;background:linear-gradient(90deg,#059669,#34d399);border-radius:99px;transition:width .4s ease}

  /* Kompakt Sensör & Durum Özeti Şeridi */
  .card{background:var(--card);border:1px solid var(--border);border-radius:15px;padding:13px;margin-bottom:10px}
  .card h3{font-size:11.5px;color:var(--sub);text-transform:uppercase;letter-spacing:.6px;margin-bottom:9px;display:flex;justify-content:space-between;align-items:center;gap:6px;flex-wrap:wrap}

  .sstrip{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin-bottom:9px}
  @media(max-width:480px){.sstrip{grid-template-columns:repeat(2,1fr)}}
  .mchip{background:var(--card2);border:1px solid var(--border);border-radius:10px;padding:7px 8px;cursor:pointer;transition:border-color .15s}
  .mchip:hover{border-color:#334c75}
  .mchip .k{font-size:10px;color:var(--sub);font-weight:600}
  .mchip .v{font-size:12.5px;font-weight:800;margin-top:2px;font-variant-numeric:tabular-nums;color:#f1f5f9}

  .smart-bar{display:flex;justify-content:space-between;align-items:center;gap:8px;background:#0b1528;border:1px solid #1e3a5f;border-radius:11px;padding:9px 11px;cursor:pointer;transition:border-color .15s}
  .smart-bar:hover{border-color:#38bdf8}
  .smart-bar .stxt{font-size:12px;font-weight:700;color:#6ee7b7}
  .smart-bar .ssub{font-size:10.5px;color:var(--sub);margin-top:2px}
  .edit-tag{font-size:11px;font-weight:800;color:#38bdf8;background:#132744;padding:5px 9px;border-radius:8px;border:1px solid #0284c755;white-space:nowrap}

  /* Tıklayınca Açılan Ayrıntı Sekmeleri (Accordion Tabs) */
  .dtabs{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:9px}
  .dtab-btn{padding:8px 6px;border-radius:9px;border:1px solid var(--border);background:var(--card2);color:#cbd5e1;font-size:11.5px;font-weight:700;cursor:pointer;text-align:center;transition:.15s}
  .dtab-btn.act{background:#172a46;color:#38bdf8;border-color:#38bdf8}
  .dpanel{display:none;margin-top:9px;padding-top:9px;border-top:1px solid var(--border)}
  .dpanel.show{display:block}

  /* 16 Hücre Matrisi */
  .cells{display:grid;grid-template-columns:repeat(4,1fr);gap:6px}
  .cell{background:var(--card2);border:1px solid var(--border);border-radius:9px;padding:6px 4px;text-align:center}
  .cell .cn{font-size:9.5px;color:var(--sub)}
  .cell .cv{font-size:12.5px;font-weight:800;margin-top:1px;font-variant-numeric:tabular-nums}
  .cell .cb{height:3.5px;background:#1e293b;border-radius:4px;margin:4px 4px 0 4px;overflow:hidden}
  .cell .cbf{height:100%;background:var(--green);border-radius:4px}
  .cell.hi{border-color:var(--amber)}.cell.hi .cv{color:var(--amber)}.cell.hi .cbf{background:var(--amber)}
  .cell.lo{border-color:var(--blue)}.cell.lo .cv{color:var(--blue)}.cell.lo .cbf{background:var(--blue)}

  /* Satırlar, Butonlar & Ayar Alt Sekmeleri */
  .row{display:flex;justify-content:space-between;align-items:center;padding:7.5px 0;border-bottom:1px solid #1f2d4788;font-size:12.5px;gap:8px}
  .row:last-child{border-bottom:none}
  .row span{color:var(--sub)}

  .sub-tabs{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-bottom:10px}
  .stab-btn{padding:9px 6px;border-radius:10px;border:1px solid var(--border);background:var(--card2);color:var(--sub);font-weight:800;font-size:12px;cursor:pointer}
  .stab-btn.act{background:#065f46;color:#ecfdf5;border-color:#10b981}
  .spanel{display:none}
  .spanel.show{display:block}

  .sec-lbl{font-size:11.5px;color:#cbd5e1;margin:11px 0 6px 0;font-weight:700;display:flex;justify-content:space-between;align-items:center}
  .sec-lbl small{color:var(--green);font-weight:700}
  .btns{display:flex;flex-wrap:wrap;gap:6px}
  button{flex:1;min-width:60px;padding:9px 7px;border:1px solid #2d3f5e;border-radius:9px;background:#18243a;color:#f8fafc;font-weight:700;font-size:12px;cursor:pointer;transition:transform .1s,background .15s}
  button:active{transform:scale(.97)}
  button.act{background:#059669;border-color:#34d399;box-shadow:0 0 10px #10b98133}
  button.blu{background:#0284c7;border-color:#38bdf8}
  button.pur{background:#6d28d9;border-color:#8b5cf6}
  button.sm{flex:0 0 auto;padding:6px 11px;min-width:52px}

  .v-edit{display:flex;align-items:center;gap:6px}
  .v-cur{font-size:12px;font-weight:800;color:var(--green);min-width:44px;text-align:right}
  input[type=number]{width:68px;padding:6px 7px;border-radius:8px;border:1px solid #334155;background:#080e1a;color:#fff;text-align:right;font-weight:700;font-size:12.5px}
  .switch{width:42px;height:22px;accent-color:var(--green);cursor:pointer}

  /* İstatistikler & Grafik (En Altta Derli Toplu) */
  .sgrid4{display:grid;grid-template-columns:1fr 1fr;gap:7px;margin-bottom:9px}
  @media(min-width:540px){.sgrid4{grid-template-columns:repeat(4,1fr)}}
  .scard{background:var(--card2);border:1px solid var(--border);border-radius:11px;padding:9px 8px;text-align:center}
  .scard .sl{font-size:10px;color:var(--sub);font-weight:700}
  .scard .sv{font-size:15px;font-weight:800;margin-top:3px;font-variant-numeric:tabular-nums}
  .scard .ss{font-size:9.5px;color:#64748b;margin-top:2px;font-variant-numeric:tabular-nums}

  .legend{display:flex;flex-wrap:wrap;gap:5px;font-size:10.5px;color:#cbd5e1;margin:7px 0;justify-content:center}
  .lg{display:inline-flex;align-items:center;gap:5px;padding:3.5px 8px;border-radius:99px;background:#0c1424;border:1px solid #2d3f5e;cursor:pointer;font-weight:600}
  .lg.off{opacity:.38;border-color:#182338}
  .dot{width:8px;height:8px;border-radius:3px;display:inline-block}
  .chart-wrap{background:var(--card2);border:1px solid var(--border);border-radius:12px;padding:7px 5px 5px 5px}
  canvas{display:block;width:100%;height:220px;cursor:pointer;touch-action:pan-y}
  .chart-tip{margin-top:7px;padding:7px 10px;background:#0b1325;border:1px solid #1e3a5f;border-radius:9px;font-size:11px;color:#e2e8f0;text-align:center;font-variant-numeric:tabular-nums}
  .dtable-wrap{margin-top:8px;max-height:195px;overflow-y:auto;border:1px solid var(--border);border-radius:10px;background:var(--card2)}
  .dtable{width:100%;border-collapse:collapse;font-size:11px;font-variant-numeric:tabular-nums}
  .dtable th{position:sticky;top:0;background:#152036;color:#94a3b8;font-weight:700;padding:6px 5px;text-align:right;border-bottom:1px solid var(--border)}
  .dtable th:first-child,.dtable td:first-child{text-align:left;padding-left:8px}
  .dtable td{padding:5.5px 5px;text-align:right;border-bottom:1px solid #1f2d4755;color:#e2e8f0}
  .dtable tr:last-child td{border-bottom:none}
  .dtable tr.hi-row{background:#1e3a5f44}

  .toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:#065f46;color:#ecfdf5;padding:9px 16px;border-radius:99px;font-size:12.5px;font-weight:700;box-shadow:0 8px 24px rgba(0,0,0,.5);opacity:0;pointer-events:none;transition:opacity .22s;z-index:99;border:1px solid #10b981;white-space:nowrap}
  .toast.show{opacity:1}
</style>
</head>
<body>

<!-- ÜST DURUM BAŞLIĞI -->
<div class="hdr">
  <div>
    <div class="title">⚡ Akıllı İnverter &amp; Daly BMS</div>
    <div class="sub" id="subInfo">10×550W Solar • 48V 280Ah LiFePO4 • Bağlanıyor...</div>
  </div>
  <div class="pills">
    <span id="mqttPill" class="pill warn">...</span>
    <span id="bmsPill" class="pill no">BMS</span>
    <span id="invPill" class="pill no">İNV</span>
  </div>
</div>

<!-- ANA SEKME GEÇİŞİ: CANLI EKRAN / AYARLAR -->
<div class="main-nav">
  <button id="navLive" class="nav-btn act" onclick="openMainView('live')">📊 Canlı Ekran</button>
  <button id="navSet" class="nav-btn" onclick="openMainView('settings')">⚙️ Ayarlar &amp; Kontrol</button>
</div>

<!-- ==================== 1. GÖRÜNÜM: CANLI EKRAN (SADE & DERLİ TOPLU) ==================== -->
<div id="viewLive" class="view show">

  <!-- 4'LÜ ANA SENSÖR KARTLARI (Üstüne basınca ilgili ayrıntıyı / ayarı açar) -->
  <div class="flow">
    <div class="fcard" onclick="toggleDetail('cells')">
      <div class="top"><span class="lbl">🔋 Akü (BMS)</span><span class="hint">16 Hücre ▾</span></div>
      <div class="val" style="color:var(--green)" id="soc">%--.-</div>
      <div class="det" id="bmsVW">--.- V • --.- A (--- W)</div>
      <div class="det2" id="bmsAhSub">--- Ah / 280 Ah • -- Döngü</div>
      <div class="soc-bar-bg"><div id="socBar" class="soc-bar-fill" style="width:0%"></div></div>
    </div>

    <div class="fcard" onclick="scrollToChart()">
      <div class="top"><span class="lbl">☀️ Solar Paneller</span><span class="hint" id="pvDayHint">Bugün: -- kWh</span></div>
      <div class="val" style="color:var(--amber)" id="pvW">--- W</div>
      <div class="det" id="pvVA">---.- V • --.- A</div>
      <div class="det2" id="pvSub">10×550W MPPT Girişi</div>
    </div>

    <div class="fcard" onclick="toggleDetail('inv')">
      <div class="top"><span class="lbl">🏠 Ev / AC Çıkış</span><span class="hint">Detay ▾</span></div>
      <div class="val" style="color:var(--blue)" id="loadW">--- W</div>
      <div class="det" id="loadDet">Yük: %-- • Çıkış: ---V</div>
      <div class="det2" id="loadDayHint">Bugün Çıkış: -- kWh</div>
    </div>

    <div class="fcard" onclick="openSettingsTab('inv')">
      <div class="top"><span class="lbl">🔌 Mod &amp; Şebeke</span><span class="hint">⚙️ Ayar ▸</span></div>
      <div class="val" style="font-size:15.5px;margin-top:6px;color:var(--purple)" id="modeShort">Bekleniyor</div>
      <div class="det" id="gridDet">TEDAŞ: --- V / -- Hz</div>
      <div class="det2" id="prioSub">SBU • OSO • Maks --A</div>
    </div>
  </div>

  <!-- TÜM KRİTİK VOLTAJ / SICAKLIK SENSÖRLERİ + TIKLAYINCA AÇILAN AYRINTI SEKMELERİ -->
  <div class="card">
    <h3>
      <span>📡 Canlı Sensörler &amp; Durum</span>
      <div style="display:flex;gap:5px;flex-wrap:wrap">
        <span id="diffBadge" class="pill ok" onclick="toggleDetail('cells')" style="cursor:pointer">Δ -- mV</span>
        <span id="invTemp" class="pill ok" onclick="openSettingsTab('auto')" style="cursor:pointer">🌡️ İnv: --°C</span>
      </div>
    </h3>

    <div class="sstrip">
      <div class="mchip" onclick="toggleDetail('cells')">
        <div class="k">En Yüksek / Düşük Hücre</div>
        <div class="v" id="minMax">-.--- / -.---V</div>
      </div>
      <div class="mchip" onclick="toggleDetail('bms')">
        <div class="k">Pil Isısı &amp; MOSFET</div>
        <div class="v" id="mos">--°C • Açık</div>
      </div>
      <div class="mchip" onclick="openSettingsTab('inv')">
        <div class="k">Aktif Şarj Hedefi</div>
        <div class="v" id="actVoltAmp">--.-V • --A</div>
      </div>
      <div class="mchip" onclick="toggleDetail('bms')">
        <div class="k">BMS Alarm / Durum</div>
        <div class="v" id="bmsFault" style="color:#34d399">Arıza Yok</div>
      </div>
    </div>

    <!-- Üstüne Basınca Otomasyon Ayar Sekmesini Açan Akıllı Durum Çubuğu -->
    <div class="smart-bar" onclick="openSettingsTab('auto')">
      <div>
        <div class="stxt" id="smartMsg">Otomasyon Hazır</div>
        <div class="ssub" id="lastCmd">Son komut: ---</div>
      </div>
      <span class="edit-tag" id="capQuickTag">⚙️ %90 Ayarla ▸</span>
    </div>

    <!-- AYRINTI SEKMELERİ (Basınca Açılır, Tekrar Basınca Kapanır!) -->
    <div class="dtabs">
      <button id="dtab_cells" class="dtab-btn" onclick="toggleDetail('cells')">🔢 16 Hücre Voltajı</button>
      <button id="dtab_inv"   class="dtab-btn" onclick="toggleDetail('inv')">⚡ İnverter Ayrıntı</button>
      <button id="dtab_bms"   class="dtab-btn" onclick="toggleDetail('bms')">🛡️ BMS Koruma</button>
    </div>

    <!-- Ayrıntı Paneli 1: 16 Hücre -->
    <div id="dpanel_cells" class="dpanel">
      <div class="cells" id="cellsGrid"></div>
    </div>

    <!-- Ayrıntı Paneli 2: İnverter Canlı Parametre Özeti -->
    <div id="dpanel_inv" class="dpanel">
      <div class="row"><span>Çıkış / Şarj Önceliği</span><strong id="roPrio">SBU / OSO</strong></div>
      <div class="row"><span>Bulk / Float / Kesme Voltajı</span><strong id="roVolts">56.8V / 56.0V / 45.0V</strong></div>
      <div class="row"><span>Şebekeye Geçiş / Aküye Dönüş V</span><strong id="roGridV">46.0V / 53.0V</strong></div>
      <div class="row"><span>Maks Şarj / Şebeke Şarj Akımı</span><strong id="roAmps">100A / 10A</strong></div>
      <div class="row"><span>İnverter Şarj / Deşarj Akımı</span><strong id="roInvCur">0A / 0A</strong></div>
      <div class="btns" style="margin-top:8px">
        <button class="blu" onclick="openSettingsTab('inv')">⚙️ İnverter Ayarlarını Düzenle ▸</button>
      </div>
    </div>

    <!-- Ayrıntı Paneli 3: Daly BMS İç Limitler Özeti -->
    <div id="dpanel_bms" class="dpanel">
      <div class="row"><span>Kapasite / Toplam Döngü</span><strong id="bsCapCyc">280 Ah • -- Döngü</strong></div>
      <div class="row"><span>1A Aktif Balans Eşiği</span><strong id="bsBal">3.20V / 20mV</strong></div>
      <div class="row"><span>Hücre Yüksek Kesme (OVP)</span><strong id="bsOvp">3.550V / 3.600V</strong></div>
      <div class="row"><span>Hücre Düşük Kesme (UVP)</span><strong id="bsUvp">2.900V / 2.850V</strong></div>
      <div class="row"><span>Paket Kesme (OVP / UVP)</span><strong id="bsPack">57.6V / 45.6V</strong></div>
      <div class="row"><span>Şarj / Deşarj Akım Sınırı</span><strong id="bsOcp">200A / 200A</strong></div>
      <div class="row"><span>Sıcaklık Limiti / Uyku</span><strong id="bsTemp">0–55°C • 60 dk</strong></div>
      <div class="btns" style="margin-top:8px">
        <button class="blu" onclick="openSettingsTab('bms')">⚙️ BMS &amp; MOSFET Ayarlarını Aç ▸</button>
      </div>
    </div>
  </div>

  <!-- EN ALTTA: ENERJİ İSTATİSTİKLERİ & GRAFİK -->
  <div class="card" id="chartCard">
    <h3><span>📊 Enerji İstatistikleri &amp; Grafik</span><span id="statDate" class="pill info">--.--.----</span></h3>

    <div class="btns" style="margin-bottom:9px">
      <button id="tabToday" class="act" onclick="setChartTab('today')">☀️ Bugün (24 Saat)</button>
      <button id="tabMonth" onclick="setChartTab('month')">📅 Son 30 Gün</button>
    </div>

    <!-- Sadeleştirilmiş 4 Net Özet Kutusu (Boş sıfır kalabalığı yok!) -->
    <div class="sgrid4">
      <div class="scard">
        <div class="sl">☀️ Solar Üretim</div>
        <div class="sv" style="color:#fbbf24" id="stPv">0.00 kWh</div>
        <div class="ss" id="stPvSub">Panel Girişi</div>
      </div>
      <div class="scard">
        <div class="sl">🏠 Ev / AC Çıkış</div>
        <div class="sv" style="color:#38bdf8" id="stLoad">0.00 kWh</div>
        <div class="ss" id="stLoadSub">Toplam Ev Yükü</div>
      </div>
      <div class="scard">
        <div class="sl">🔋 Akü Dolum / Deşarj</div>
        <div class="sv" style="color:#10b981;font-size:13.5px" id="stBatBoth">+0.00 / -0.00</div>
        <div class="ss" id="stBatSub">kWh Giren / Çıkan</div>
      </div>
      <div class="scard">
        <div class="sl">🔌 TEDAŞ Şebeke</div>
        <div class="sv" style="color:#fb923c" id="stGridTot">0.00 kWh</div>
        <div class="ss" id="stGridSub">Bypass: 0.00 • Şarj: 0.00</div>
      </div>
    </div>

    <div class="row" style="padding:2px 0 4px 0;border-bottom:none">
      <span>☀️ Güneşten Karşılama Oranı</span>
      <strong id="stSolarPct" style="color:#34d399">%100.0</strong>
    </div>
    <div class="soc-bar-bg" style="height:7px;margin:2px 0 5px 0"><div id="solarBar" class="soc-bar-fill" style="width:100%"></div></div>
    <div id="stAvgLine" style="font-size:10.5px;color:var(--sub);text-align:right;margin-bottom:4px"></div>

    <div class="legend">
      <span class="lg" id="lg_pv"   onclick="toggleSeries('pv')"><i class="dot" style="background:#fbbf24"></i>Solar</span>
      <span class="lg" id="lg_load" onclick="toggleSeries('load')"><i class="dot" style="background:#38bdf8"></i>Ev / AC Çıkış</span>
      <span class="lg" id="lg_chg"  onclick="toggleSeries('chg')"><i class="dot" style="background:#10b981"></i>Akü Dolum</span>
      <span class="lg off" id="lg_dis" onclick="toggleSeries('dis')"><i class="dot" style="background:#2dd4bf"></i>Akü Deşarj</span>
      <span class="lg" id="lg_grid" onclick="toggleSeries('grid')"><i class="dot" style="background:#fb923c"></i>TEDAŞ</span>
    </div>

    <div class="chart-wrap">
      <canvas id="energyChart" height="220"></canvas>
    </div>

    <div id="chartTip" class="chart-tip">Grafikte herhangi bir saate veya güne dokunarak kWh detayını görün</div>

    <div id="dayTableWrap" class="dtable-wrap" style="display:none">
      <table class="dtable">
        <thead><tr><th>Tarih</th><th>☀️ Solar</th><th>🏠 Ev Çıkış</th><th>🔋 Dol/Deş</th><th>🔌 TEDAŞ</th></tr></thead>
        <tbody id="dayTableBody"></tbody>
      </table>
    </div>
  </div>
</div>

<!-- ==================== 2. GÖRÜNÜM: AYARLAR SEKMESİ (3 ALT SEKME) ==================== -->
<div id="viewSettings" class="view">

  <div class="sub-tabs">
    <button id="stab_auto" class="stab-btn act" onclick="openSettingsTab('auto')">🤖 Otomasyon</button>
    <button id="stab_inv"  class="stab-btn" onclick="openSettingsTab('inv')">⚡ İnverter</button>
    <button id="stab_bms"  class="stab-btn" onclick="openSettingsTab('bms')">🛡️ BMS &amp; Sistem</button>
  </div>

  <!-- ALT SEKME 1: OTOMASYON, BOOST & CUT, TERMAL KORUMA, AYLIK BALANS -->
  <div id="spanel_auto" class="spanel show">
    <div class="card">
      <h3><span>🤖 Boost &amp; Cut + Termal Otomasyon</span><span id="capBadge" class="pill ok">Üst Sınır: %90</span></h3>

      <div class="sec-lbl" style="margin-top:2px"><span>🛑 Hızlı Üst Şarj Sınırı Seçimi</span><small id="lblMaxSoc">%90</small></div>
      <div class="btns" style="margin-bottom:10px">
        <button id="btnCap80" onclick="setMaxSoc(80)">%80</button>
        <button id="btnCap90" onclick="setMaxSoc(90)">%90 (Ömür)</button>
        <button id="btnCap92" onclick="setMaxSoc(92)">%92</button>
        <button id="btnCap95" onclick="setMaxSoc(95)">%95</button>
        <button id="btnCap100" onclick="setMaxSoc(100)">%100 (Tam)</button>
      </div>

      <div class="row"><span>Otomatik BMS &amp; Termal Yönetim</span><input class="switch" type="checkbox" id="cAuto" checked></div>
      <div class="row"><span>🛑 Maksimum Şarj Sınırı (%)</span><input type="number" id="cMaxS" value="90" min="50" max="100" step="1"></div>
      <div class="row"><span>⚡ Boost Bulk Voltajı (PCVV) (V)</span><input type="number" id="cBulkV" value="56.8" min="52.0" max="57.2" step="0.1"></div>
      <div class="row"><span>⚡ Boost Float Voltajı (PBFT) (V)</span><input type="number" id="cFloatV" value="56.0" min="51.0" max="56.8" step="0.1"></div>
      <div class="row"><span>Hızlı Boost Şarj Akımı (A)</span><input type="number" id="cMaxA" value="100" step="10"></div>
      <div class="row"><span>Yumuşatma / Termal-1 Hızı (A)</span><input type="number" id="cMidA" value="40" step="10"></div>
      <div class="row"><span>Balans / Termal-2 Hızı (A)</span><input type="number" id="cBalA" value="10" step="10"></div>
      <div class="row"><span>Voltaj Düşürme Hücre Eşiği (V)</span><input type="number" id="cTap1" value="3.45" step="0.01"></div>
      <div class="row"><span>1A Balans Hücre Eşiği (V)</span><input type="number" id="cTap2" value="3.49" step="0.01"></div>

      <div class="sec-lbl"><span>🔌 Düşük Akü Şebeke &amp; Şebeke Şarjı</span></div>
      <div class="row"><span>Şebekeye Geçiş (% SoC Altı)</span><input type="number" id="cSocG" value="20" step="1"></div>
      <div class="row"><span>Şebeke Şarjı Aç (% SoC Altı)</span><input type="number" id="cChgOn" value="20" step="1"></div>
      <div class="row"><span>Şebeke Şarjı Kapat (% SoC Üstü)</span><input type="number" id="cChgOff" value="25" step="1"></div>
      <div class="row"><span>Aküye (SBU) Dönüş (% SoC Üstü)</span><input type="number" id="cSocB" value="30" step="1"></div>

      <div class="sec-lbl"><span>🌡️ İnverter Aşırı Isınma Koruması</span><span id="thermBadge" class="pill ok">Normal</span></div>
      <div class="row"><span>🌡️ Kademe-1 Kısma → 40A (°C)</span><input type="number" id="cTWarn" value="65" min="45" max="78" step="1"></div>
      <div class="row"><span>🔥 Kademe-2 Kısma → 10A (°C)</span><input type="number" id="cTHigh" value="72" min="50" max="84" step="1"></div>
      <div class="row"><span>🚨 Kademe-3 Acil Bypass (°C)</span><input type="number" id="cTByp" value="78" min="55" max="90" step="1"></div>

      <div class="sec-lbl"><span>📅 Aylık Otomatik %100 Balans Günü</span><span id="mBalBadge" class="pill info">Ayın 1'i</span></div>
      <div class="row"><span>Ayda 1 Oto %100 Balans &amp; Kalibrasyon</span><input class="switch" type="checkbox" id="cMBal" checked></div>
      <div class="row"><span>Balans Günü (Ayın 1–28'i)</span><input type="number" id="cBalDay" value="1" min="1" max="28" step="1"></div>
      <div class="btns" style="margin-top:6px">
        <button id="btnMBalToggle" onclick="toggleMonthlyBal()">🔄 Bu Ay %100 Balansı Şimdi Başlat</button>
      </div>

      <div class="btns" style="margin-top:12px">
        <button class="act" onclick="saveCfg()">💾 Tüm Otomasyon Ayarlarını Kaydet</button>
      </div>
    </div>
  </div>

  <!-- ALT SEKME 2: İNVERTER TÜM WATCHPOWER AYARLARI -->
  <div id="spanel_inv" class="spanel">
    <div class="card">
      <h3><span>⚡ İnverter Ayarları (WatchPower)</span><span id="invRatedBadge" class="pill info">5500W • 48V</span></h3>

      <div class="sec-lbl"><span>Prog 01 — Çıkış Önceliği</span><small id="lblOutPrio">---</small></div>
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

      <div class="sec-lbl"><span>Prog 02 — Maks. Toplam Şarj Akımı</span><small id="lblMaxChg">-- A</small></div>
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
        <button id="btnPBT01" onclick="pubCmd('PBT01')">FLD</button>
        <button id="btnPBT02" onclick="pubCmd('PBT02')">USE (LiFePO4)</button>
      </div>

      <div class="sec-lbl"><span>Prog 03 — AC Giriş Aralığı</span><small id="lblAcRange">---</small></div>
      <div class="btns">
        <button id="btnPGR00" onclick="pubCmd('PGR00')">APL (90-280V)</button>
        <button id="btnPGR01" onclick="pubCmd('PGR01')">UPS (170-280V)</button>
      </div>

      <div class="sec-lbl" style="margin-top:12px"><span>Manuel Voltaj Ayarları</span></div>
      <div class="row"><span>Prog 26 • Bulk (PCVV)</span><div class="v-edit"><span class="v-cur" id="curBulk">--.-V</span><input type="number" id="inBulk" value="56.8" step="0.1"><button class="sm blu" onclick="sendV('PCVV','inBulk')">Yaz</button></div></div>
      <div class="row"><span>Prog 27 • Float (PBFT)</span><div class="v-edit"><span class="v-cur" id="curFloat">--.-V</span><input type="number" id="inFloat" value="56.0" step="0.1"><button class="sm blu" onclick="sendV('PBFT','inFloat')">Yaz</button></div></div>
      <div class="row"><span>Prog 29 • Kesme (PSDV)</span><div class="v-edit"><span class="v-cur" id="curCut">--.-V</span><input type="number" id="inCut" value="45.0" step="0.1"><button class="sm blu" onclick="sendV('PSDV','inCut')">Yaz</button></div></div>
      <div class="row"><span>Prog 12 • Şebekeye Dönüş (PBCV)</span><div class="v-edit"><span class="v-cur" id="curBGrid">--.-V</span><input type="number" id="inBGrid" value="46.0" step="0.5"><button class="sm blu" onclick="sendV('PBCV','inBGrid')">Yaz</button></div></div>
      <div class="row"><span>Prog 13 • Aküye Dönüş (PBDV)</span><div class="v-edit"><span class="v-cur" id="curBBat">--.-V</span><input type="number" id="inBBat" value="53.0" step="0.5"><button class="sm blu" onclick="sendV('PBDV','inBBat')">Yaz</button></div></div>

      <div class="sec-lbl"><span>Sistem Anahtarları (QFLAG)</span></div>
      <div class="btns">
        <button id="btnBuz" onclick="toggleFlag('a', curSet.buz)">🔔 Buzzer</button>
        <button id="btnByp" onclick="toggleFlag('b', curSet.bypass)">🔄 Bypass</button>
        <button id="btnPwr" onclick="toggleFlag('j', curSet.pwrsav)">🌙 Tasarruf</button>
        <button id="btnLcd" onclick="toggleFlag('x', curSet.lcd)">💡 LCD</button>
      </div>

      <div class="btns" style="margin-top:12px">
        <button class="pur" onclick="applyLiFePO4()">⚡ Tek Tuşla 16S LiFePO4 Boost Profilini Yaz</button>
      </div>
    </div>
  </div>

  <!-- ALT SEKME 3: DALY BMS KONTROL & SİSTEM BAKIM -->
  <div id="spanel_bms" class="spanel">
    <div class="card">
      <h3><span>🛡️ Daly BMS Kontrol &amp; Sistem</span><span id="bmsConnBadge" class="pill ok">RS485</span></h3>

      <div class="sec-lbl" style="margin-top:2px"><span>⚡ BMS MOSFET Anahtarları (Dikkatli Kullanın)</span><small id="lblMosState">AÇIK / AÇIK</small></div>
      <div class="btns" style="margin-bottom:10px">
        <button id="btnChgMos" onclick="toggleBmsMos('CHGMOS', curBms.chgMos)">⚡ Şarj MOSFET</button>
        <button id="btnDisMos" onclick="toggleBmsMos('DISMOS', curBms.disMos)">🔌 Deşarj MOSFET</button>
      </div>

      <div class="sec-lbl"><span>⚖️ 1A Aktif Balans Eşiği (RS485)</span><small>Önerilen: 3.40V / 15mV</small></div>
      <div class="row">
        <span>Başlama (V) &amp; Fark (mV)</span>
        <div class="v-edit">
          <input type="number" id="inBalV" value="3.40" step="0.01" style="width:62px">
          <input type="number" id="inBalMv" value="15" step="5" style="width:52px">
          <button class="sm blu" onclick="saveBmsBal()">Yaz</button>
        </div>
      </div>

      <div class="sec-lbl" style="margin-top:14px"><span>🗑️ İstatistik Bakımı</span></div>
      <div class="btns">
        <button onclick="resetStats()" style="color:#fca5a5;border-color:#7f1d1d">🗑️ Günlük ve 30 Günlük kWh Sayaçlarını Sıfırla</button>
      </div>
    </div>
  </div>

  <div class="btns">
    <button onclick="openMainView('live')" style="padding:11px">← Canlı Ekrana Dön</button>
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
let openDetailKey = '';
let chartTab = 'today';
let selChartIdx = -1;
let chartGeom = { padL: 42, cW: 300, n: 1, rows: [] };
let seriesOn = { pv: true, load: true, chg: true, dis: false, grid: true };

function toast(msg){
  const el = document.getElementById('toast');
  el.innerText = msg;
  el.classList.add('show');
  setTimeout(()=>el.classList.remove('show'), 2500);
}

function setActive(ids, activeId){
  ids.forEach(id => {
    const el = document.getElementById(id);
    if(el) el.className = (id === activeId) ? 'act' : '';
  });
}

/* Ana Görünüm & Alt Sekme Yönetimi */
function openMainView(which){
  const isLive = (which === 'live');
  document.getElementById('viewLive').className = isLive ? 'view show' : 'view';
  document.getElementById('viewSettings').className = isLive ? 'view' : 'view show';
  document.getElementById('navLive').className = isLive ? 'nav-btn act' : 'nav-btn';
  document.getElementById('navSet').className  = isLive ? 'nav-btn' : 'nav-btn act';
  if(isLive) requestAnimationFrame(drawChart);
  window.scrollTo({ top: 0, behavior: 'smooth' });
}

function openSettingsTab(tabKey){
  openMainView('settings');
  ['auto','inv','bms'].forEach(k => {
    document.getElementById('stab_' + k).className   = (k === tabKey) ? 'stab-btn act' : 'stab-btn';
    document.getElementById('spanel_' + k).className = (k === tabKey) ? 'spanel show' : 'spanel';
  });
}

function toggleDetail(key){
  openDetailKey = (openDetailKey === key) ? '' : key;
  ['cells','inv','bms'].forEach(k => {
    document.getElementById('dtab_' + k).className   = (k === openDetailKey) ? 'dtab-btn act' : 'dtab-btn';
    document.getElementById('dpanel_' + k).className = (k === openDetailKey) ? 'dpanel show' : 'dpanel';
  });
}

function scrollToChart(){
  document.getElementById('chartCard').scrollIntoView({ behavior: 'smooth' });
}

function connectMqtt(){
  if(IS_LOCAL_ESP){
    document.getElementById('mqttPill').className = 'pill ok';
    document.getElementById('mqttPill').innerText = 'WI-FI';
    pollLocal();
    pollLocalStats();
    setInterval(pollLocal, 2500);
    setInterval(pollLocalStats, 12000);
    return;
  }
  const cid = 'APK_' + Math.random().toString(16).slice(2, 10);
  client = mqtt.connect(MQTT_URL, { clientId: cid, clean: true, reconnectPeriod: 3000, connectTimeout: 10000 });
  client.on('connect', () => {
    document.getElementById('mqttPill').className = 'pill ok';
    document.getElementById('mqttPill').innerText = 'MQTT';
    client.subscribe(T_STATUS);
    client.subscribe(T_STATS);
  });
  client.on('reconnect', () => {
    document.getElementById('mqttPill').className = 'pill warn';
    document.getElementById('mqttPill').innerText = 'BAĞLANIYOR';
  });
  client.on('error', () => {
    document.getElementById('mqttPill').className = 'pill no';
    document.getElementById('mqttPill').innerText = 'HATA';
  });
  client.on('message', (topic, message) => {
    try{
      if(topic === T_STATUS) renderData(JSON.parse(message.toString()));
      else if(topic === T_STATS){ curStats = JSON.parse(message.toString()); renderStats(); }
    }catch(e){}
  });
}

async function pollLocal(){
  try{ const r = await fetch('/api/status'); renderData(await r.json()); }catch(e){}
}

async function pollLocalStats(){
  try{ const r = await fetch('/api/stats'); curStats = await r.json(); renderStats(); }catch(e){}
}

function renderData(d){
  const b = d.bms || {}, bs = d.bmsSet || {}, i = d.inv || {}, s = d.set || {}, c = d.cfg || {};
  curSet = s;
  curBms = b;
  document.getElementById('subInfo').innerText = '10×550W • 280Ah LiFePO4 • Son: ' + new Date().toLocaleTimeString('tr-TR');

  document.getElementById('bmsPill').className = 'pill ' + (b.ok?'ok':'no');
  document.getElementById('bmsPill').innerText = 'BMS: ' + (b.ok?'OK':'YOK');
  document.getElementById('invPill').className = 'pill ' + (i.ok?'ok':'no');
  document.getElementById('invPill').innerText = 'İNV: ' + (i.ok?'OK':'YOK');

  // 1) Ana Kartlar
  const soc = Number(b.soc||0);
  const stNames = ['Boşta', 'Şarj ⚡', 'Deşarj 🔋'];
  document.getElementById('soc').innerText = '%' + soc.toFixed(1);
  document.getElementById('socBar').style.width = Math.max(0, Math.min(100, soc)) + '%';
  document.getElementById('bmsVW').innerText = Number(b.v||0).toFixed(2) + 'V • ' + (b.a>=0?'+':'') + Number(b.a||0).toFixed(1) + 'A (' + (b.w||0) + 'W)';
  document.getElementById('bmsAhSub').innerText = Number(b.ah||0).toFixed(0) + '/' + Number(bs.ratedAh||280).toFixed(0) + 'Ah • ' + (b.cyc||0) + ' Döngü • ' + (stNames[b.st]||'-');

  document.getElementById('pvW').innerText = (i.pvW||0) + ' W';
  document.getElementById('pvVA').innerText = Number(i.pvV||0).toFixed(1) + ' V • ' + Number(i.pvA||0).toFixed(1) + ' A';

  document.getElementById('loadW').innerText = (i.loadW||0) + ' W';
  document.getElementById('loadDet').innerText = 'Yük: %' + (i.loadPct||0) + ' • ' + Number(i.outV||0).toFixed(0) + 'V Çıkış';

  document.getElementById('modeShort').innerText = i.mode || 'Bilinmiyor';
  document.getElementById('gridDet').innerText = 'TEDAŞ: ' + Number(i.gridV||0).toFixed(0) + 'V / ' + Number(i.gridHz||0).toFixed(1) + 'Hz';

  // 2) Sensör Şeridi & Rozetler
  document.getElementById('minMax').innerText = Number(b.maxV||0).toFixed(3) + ' / ' + Number(b.minV||0).toFixed(3) + 'V';
  document.getElementById('diffBadge').innerText = 'Δ ' + (b.diff||0) + ' mV';
  document.getElementById('diffBadge').className = 'pill ' + ((b.diff||0)>40?'warn':'ok');

  const mosOk = (b.chgMos && b.disMos);
  document.getElementById('mos').innerText = (b.tMax||0) + '°C • ' + (mosOk ? 'MOS Açık' : ('Ş:'+(b.chgMos?'1':'0')+'/D:'+(b.disMos?'1':'0')));

  const fEl = document.getElementById('bmsFault');
  fEl.innerText = b.fault || 'Arıza Yok';
  fEl.style.color = (b.fault && b.fault !== 'Arıza Yok') ? '#f87171' : '#34d399';

  const tInv = Number(i.temp||0), tLvl = Number(i.thermLvl||0);
  const invTempEl = document.getElementById('invTemp');
  const thEl = document.getElementById('thermBadge');
  if(tLvl >= 3 || i.thermByp){
    invTempEl.className = 'pill no'; invTempEl.innerText = '🚨 İnv: ' + tInv + '°C BYPASS';
    thEl.className = 'pill no';      thEl.innerText = '🚨 ' + tInv + '°C ACİL BYPASS';
  } else if(tLvl === 2){
    invTempEl.className = 'pill no'; invTempEl.innerText = '🔥 İnv: ' + tInv + '°C (10A)';
    thEl.className = 'pill no';      thEl.innerText = '🔥 ' + tInv + '°C Kısma-2';
  } else if(tLvl === 1){
    invTempEl.className = 'pill warn'; invTempEl.innerText = '⚠️ İnv: ' + tInv + '°C (40A)';
    thEl.className = 'pill warn';      thEl.innerText = '⚠️ ' + tInv + '°C Kısma-1';
  } else {
    invTempEl.className = 'pill ok'; invTempEl.innerText = '🌡️ İnv: ' + tInv + '°C';
    thEl.className = 'pill ok';      thEl.innerText = '🌡️ ' + tInv + '°C Normal';
  }

  document.getElementById('smartMsg').innerText = i.smart || 'Aktif';
  document.getElementById('lastCmd').innerText  = 'Son: ' + (i.lastCmd || '---');
  document.getElementById('roInvCur').innerText = (i.chgA||0) + ' A Şarj / ' + (i.disA||0) + ' A Deşarj';

  // 3) 16 Hücre Ayrıntı Paneli
  if(Array.isArray(b.cells) && b.cells.length === 16){
    const bMask = Number(b.balMask||0);
    let html = '';
    for(let k=0;k<16;k++){
      const v = Number(b.cells[k]||0);
      const pct = Math.max(0, Math.min(100, ((v - 2.80) / (3.65 - 2.80)) * 100));
      const isBal = ((bMask >> k) & 1) === 1;
      let cls = 'cell' + (k+1 === b.maxN ? ' hi' : (k+1 === b.minN ? ' lo' : ''));
      html += `<div class="${cls}"><div class="cn">H${k+1}${isBal?' ⚖️':''}</div><div class="cv">${v.toFixed(3)}V</div><div class="cb"><div class="cbf" style="width:${pct.toFixed(0)}%"></div></div></div>`;
    }
    document.getElementById('cellsGrid').innerHTML = html;
  }

  // 4) Daly BMS Ayrıntı & Ayar Sekmesi
  document.getElementById('bmsConnBadge').innerText = b.ip || 'RS485';
  document.getElementById('lblMosState').innerText = 'Şarj:' + (b.chgMos?'AÇIK':'KAPALI') + ' • Deş:' + (b.disMos?'AÇIK':'KAPALI');
  document.getElementById('btnChgMos').className = b.chgMos ? 'act' : '';
  document.getElementById('btnDisMos').className = b.disMos ? 'act' : '';
  if(bs && bs.ok){
    document.getElementById('bsCapCyc').innerText = Number(bs.ratedAh).toFixed(0) + ' Ah • ' + (b.cyc||0) + ' Döngü';
    document.getElementById('bsBal').innerText    = Number(bs.balV).toFixed(2) + 'V / ' + bs.balMv + 'mV';
    document.getElementById('bsOvp').innerText    = Number(bs.ovp1).toFixed(3) + 'V / ' + Number(bs.ovp2).toFixed(3) + 'V';
    document.getElementById('bsUvp').innerText    = Number(bs.uvp1).toFixed(3) + 'V / ' + Number(bs.uvp2).toFixed(3) + 'V';
    document.getElementById('bsPack').innerText   = Number(bs.pOvp).toFixed(1) + 'V / ' + Number(bs.pUvp).toFixed(1) + 'V';
    document.getElementById('bsOcp').innerText    = bs.cOcp2 + 'A / ' + bs.dOcp2 + 'A';
    document.getElementById('bsTemp').innerText   = bs.cTLo + '–' + bs.cTHi + '°C • ' + Math.round((bs.sleep||3600)/60) + ' dk';
    if(firstBmsSet){
      document.getElementById('inBalV').value  = Number(bs.balV).toFixed(2);
      document.getElementById('inBalMv').value = bs.balMv;
      firstBmsSet = false;
    }
  }

  // 5) İnverter Özeti & Ayar Sekmesi
  if(s && s.ok){
    const outShort = ['UTI','SOL','SBU'];
    const chgShort = ['UTI','CSO','SNU','OSO'];
    const outNames = ['UTI (Şebeke)', 'SOL (Önce Solar)', 'SBU (Akü+Solar)'];
    const chgNames = ['UTI', 'CSO (Önce Güneş)', 'SNU (Güneş+Şebeke)', 'OSO (Sadece Güneş)'];
    const batNames = ['AGM', 'FLD', 'USE (LiFePO4)'];
    const acNames  = ['APL (90-280V)', 'UPS (170-280V)'];

    document.getElementById('prioSub').innerText    = (outShort[s.outPrio]||'-') + ' • ' + (chgShort[s.chgPrio]||'-') + ' • Maks ' + s.maxChgA + 'A';
    document.getElementById('actVoltAmp').innerText = Number(s.bulkV).toFixed(1) + 'V • ' + s.maxChgA + 'A';
    document.getElementById('roPrio').innerText     = (outNames[s.outPrio]||'-') + ' / ' + (chgNames[s.chgPrio]||'-');
    document.getElementById('roVolts').innerText    = Number(s.bulkV).toFixed(1) + 'V / ' + Number(s.floatV).toFixed(1) + 'V / ' + Number(s.cutOffV).toFixed(1) + 'V';
    document.getElementById('roGridV').innerText    = Number(s.backGridV).toFixed(1) + 'V / ' + Number(s.backBatV).toFixed(1) + 'V';
    document.getElementById('roAmps').innerText     = s.maxChgA + ' A / ' + s.gridChgA + ' A';

    document.getElementById('invRatedBadge').innerText = (s.ratedW||5500) + 'W • 48V';
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

  // 6) Otomasyon & Aylık Balans Rozetleri
  if(c.maxSoc !== undefined){
    const ms = Math.round(Number(c.maxSoc));
    document.getElementById('capQuickTag').innerText = c.mBalAct ? '📅 %100 Balans ▸' : ('⚙️ Sınır %' + ms + ' ▸');
    document.getElementById('lblMaxSoc').innerText = c.mBalAct ? '📅 AYLIK %100 BALANS' : ((ms >= 100) ? 'Sınırsız (%100)' : ('Maks %' + ms));
    setActive(['btnCap80','btnCap90','btnCap92','btnCap95','btnCap100'], 'btnCap' + (c.mBalAct ? 100 : ms));
    const cb = document.getElementById('capBadge');
    if(c.mBalAct){ cb.className = 'pill info'; cb.innerText = '📅 AYLIK %100 BALANS'; }
    else if(c.socCap){ cb.className = 'pill warn'; cb.innerText = '🛑 %' + ms + ' DURDU (0A)'; }
    else { cb.className = 'pill ok'; cb.innerText = 'Üst Sınır: %' + ms; }
  }

  if(c.mBal !== undefined){
    window.curMBalAct = !!c.mBalAct;
    const mb = document.getElementById('mBalBadge');
    const btn = document.getElementById('btnMBalToggle');
    if(c.mBalAct){
      mb.className = 'pill warn'; mb.innerText = '⚡ %100 BALANSTA';
      btn.className = 'act'; btn.innerText = '⏹️ Aktif %100 Balansı İptal Et (%90\'a Dön)';
    } else if(!c.mBal){
      mb.className = 'pill no'; mb.innerText = 'Kapalı';
      btn.className = ''; btn.innerText = '🔄 Bu Ay %100 Balansı Şimdi Başlat';
    } else {
      mb.className = 'pill ok';
      const lm = Number(c.lastBalM||0);
      const lmStr = lm > 200000 ? ('Son: ' + String(lm).slice(4) + '/' + String(lm).slice(0,4) + ' • ') : '';
      mb.innerText = lmStr + 'Ayın ' + (c.balDay||1) + '\'i';
      btn.className = ''; btn.innerText = '🔄 Bu Ay %100 Balansı Şimdi Başlat';
    }
  }

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
    if(c.mBal  !== undefined) document.getElementById('cMBal').checked = !!c.mBal;
    if(c.balDay !== undefined) document.getElementById('cBalDay').value = c.balDay;
    firstCfg = false;
  }
}

/* Komut & Ayar Göndericiler */
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

function sendV(prefix, inputId){ pubCmd(prefix + Number(document.getElementById(inputId).value).toFixed(1)); }
function toggleFlag(ch, isEnabled){ pubCmd((isEnabled ? 'PD' : 'PE') + ch); }

function toggleBmsMos(which, isOn){
  const label = (which === 'CHGMOS') ? 'Şarj MOSFET' : 'Deşarj MOSFET';
  const next = isOn ? 0 : 1;
  if(!confirm(`DİKKAT: Daly BMS ${label} ${next?'AÇILSIN':'KAPATILSIN'} mı?`)) return;
  pubCmd(`BMS:${which}:${next}`);
}

function saveBmsBal(){
  const mv = Math.round(Number(document.getElementById('inBalV').value) * 1000);
  const diff = Math.round(Number(document.getElementById('inBalMv').value));
  if(!confirm(`Daly BMS 1A Balans ${mv}mV / ${diff}mV yazılsın mı?`)) return;
  pubCmd(`BMS:BAL:${mv}:${diff}`);
}

async function applyLiFePO4(){
  if(!confirm('İnvertere 16S LiFePO4 Boost & Cut (56.8V / 56.0V / 45.0V / 46.0V / 53.0V) yazılsın mı?')) return;
  if(IS_LOCAL_ESP){ await fetch('/api/apply_lifepo4'); toast('LiFePO4 Profili Yazıldı!'); setTimeout(pollLocal, 800); return; }
  pubCmd('APPLY_LIFEPO4');
}

function setMaxSoc(val){ document.getElementById('cMaxS').value = val; saveCfg(); }

function toggleMonthlyBal(){
  if(window.curMBalAct){
    if(!confirm('Aktif %100 balans iptal edilip %90 sınırına dönülsün mü?')) return;
    pubCmd('CANCEL_MONTHLY_BAL');
  } else {
    if(!confirm('Şimdi %100 balans günü başlatılsın mı? Tamamlanınca otomatik %90\'a döner.')) return;
    pubCmd('START_MONTHLY_BAL');
  }
}

async function saveCfg(){
  const qs = `auto=${document.getElementById('cAuto').checked?1:0}&maxS=${document.getElementById('cMaxS').value}&bulkV=${document.getElementById('cBulkV').value}&floatV=${document.getElementById('cFloatV').value}&socG=${document.getElementById('cSocG').value}&socB=${document.getElementById('cSocB').value}&chgOn=${document.getElementById('cChgOn').value}&chgOff=${document.getElementById('cChgOff').value}&maxA=${document.getElementById('cMaxA').value}&midA=${document.getElementById('cMidA').value}&balA=${document.getElementById('cBalA').value}&tap1=${document.getElementById('cTap1').value}&tap2=${document.getElementById('cTap2').value}&tWarn=${document.getElementById('cTWarn').value}&tHigh=${document.getElementById('cTHigh').value}&tByp=${document.getElementById('cTByp').value}&mBal=${document.getElementById('cMBal').checked?1:0}&balDay=${document.getElementById('cBalDay').value}`;
  if(IS_LOCAL_ESP){ await fetch('/api/cfg?' + qs); toast('Ayarlar kaydedildi!'); pollLocal(); return; }
  if(!client || !client.connected){ toast('MQTT bağlantısı bekleniyor...'); return; }
  client.publish(T_CFG, qs);
  toast('Ayarlar ESP32\'ye kaydedildi!');
}

/* ==================== İSTATİSTİK & GRAFİK MOTORU ==================== */
function setChartTab(tab){
  chartTab = tab;
  selChartIdx = -1;
  document.getElementById('tabMonth').className = (tab === 'month') ? 'act' : '';
  document.getElementById('tabToday').className = (tab === 'today') ? 'act' : '';
  renderStats();
}

function toggleSeries(key){
  seriesOn[key] = !seriesOn[key];
  if(!Object.values(seriesOn).some(v => v)) seriesOn[key] = true;
  const el = document.getElementById('lg_' + key);
  if(el) el.className = seriesOn[key] ? 'lg' : 'lg off';
  drawChart();
}

async function resetStats(){
  if(!confirm('Tüm günlük ve 30 günlük kWh enerji sayaçları sıfırlansın mı?')) return;
  if(IS_LOCAL_ESP){ await fetch('/api/stats_reset'); pollLocalStats(); toast('Sayaçlar sıfırlandı!'); return; }
  pubCmd('RESET_STATS');
}

function niceMax(v){
  if(v <= 0.2) return 0.4;
  const raw = v * 1.15;
  const steps = [0.4, 0.8, 1.2, 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 30, 40, 50, 60, 80, 100];
  for(const s of steps) if(raw <= s) return s;
  return Math.ceil(raw / 20) * 20;
}

function renderStats(){
  if(!curStats) return;
  document.getElementById('statDate').innerText = curStats.dt || '--.--.----';

  // Üst kartlardaki "Bugün kWh" küçük ipuçlarını da besle
  const td = curStats.today || {};
  document.getElementById('pvDayHint').innerText   = 'Bugün: ' + Number(td.pv||0).toFixed(2) + ' kWh';
  document.getElementById('loadDayHint').innerText = 'Bugün Ev Çıkışı: ' + Number(td.load||0).toFixed(2) + ' kWh';

  const isMonth = (chartTab === 'month');
  const src = isMonth ? (curStats.month || {}) : td;
  const dec = isMonth ? 2 : 2;
  const pv = Number(src.pv||0), ld = Number(src.load||0), ch = Number(src.chg||0),
        ds = Number(src.dis||0), go = Number(src.gridOut||0), gc = Number(src.gridChg||0);
  const gridTot = go + gc;

  document.getElementById('stPv').innerText      = pv.toFixed(dec) + ' kWh';
  document.getElementById('stLoad').innerText    = ld.toFixed(dec) + ' kWh';
  document.getElementById('stBatBoth').innerHTML = `+${ch.toFixed(dec)} / <span style="color:#2dd4bf">-${ds.toFixed(dec)}</span>`;
  document.getElementById('stGridTot').innerText = gridTot.toFixed(dec) + ' kWh';
  document.getElementById('stGridSub').innerText = `Ev: ${go.toFixed(2)} • Şarj: ${gc.toFixed(2)}`;

  const solarShare = ld > 0.001 ? Math.max(0, Math.min(100, ((ld - Math.min(ld, gridTot)) / ld) * 100)) : 100;
  document.getElementById('stSolarPct').innerText = '%' + solarShare.toFixed(1) + ' Güneş / Akü';
  document.getElementById('solarBar').style.width = solarShare.toFixed(1) + '%';

  const days = curStats.days || [];
  const dCnt = Math.max(Number(curStats.daysCnt || days.length || 1), 1);
  const avgEl = document.getElementById('stAvgLine');
  if(isMonth){
    avgEl.innerText = `${dCnt} Gün • Günlük Ort: ☀️ ${(pv/dCnt).toFixed(2)} kWh | 🏠 ${(ld/dCnt).toFixed(2)} kWh | 🔌 ${(gridTot/dCnt).toFixed(2)} kWh`;
  } else {
    const netBat = ch - ds;
    avgEl.innerText = `Bugün Net Akü: ${netBat >= 0 ? '+' : ''}${netBat.toFixed(2)} kWh • TEDAŞ Kullanımı: ${gridTot.toFixed(2)} kWh`;
  }

  const tblWrap = document.getElementById('dayTableWrap');
  if(isMonth && days.length > 0){
    tblWrap.style.display = 'block';
    let rowsHtml = '';
    for(let k = days.length - 1; k >= 0; k--){
      const d = days[k];
      const hasDis = d.length >= 7;
      const dPv = Number(d[1]||0), dLd = Number(d[2]||0), dCh = Number(d[3]||0);
      const dDs = hasDis ? Number(d[4]||0) : 0;
      const dGr = hasDis ? (Number(d[5]||0) + Number(d[6]||0)) : (Number(d[4]||0) + Number(d[5]||0));
      const isToday = (k === days.length - 1);
      rowsHtml += `<tr class="${isToday?'hi-row':''}" onclick="selectChartIdx(${k})">
        <td><b>${d[0]}</b>${isToday?' <small style="color:#34d399">(Bugün)</small>':''}</td>
        <td style="color:#fbbf24">${dPv.toFixed(2)}</td>
        <td style="color:#38bdf8">${dLd.toFixed(2)}</td>
        <td style="color:#10b981">+${dCh.toFixed(1)} / <span style="color:#2dd4bf">-${dDs.toFixed(1)}</span></td>
        <td style="color:${dGr>0.01?'#fb923c':'#64748b'}">${dGr.toFixed(2)}</td>
      </tr>`;
    }
    document.getElementById('dayTableBody').innerHTML = rowsHtml;
  } else {
    tblWrap.style.display = 'none';
  }

  drawChart();
}

function selectChartIdx(idx){ selChartIdx = idx; drawChart(); }

function updateChartTip(rows, idx){
  const el = document.getElementById('chartTip');
  if(!rows || rows.length === 0){ el.innerText = 'Henüz kayıtlı enerji verisi yok'; return; }
  const k = (idx >= 0 && idx < rows.length) ? idx : (rows.length - 1);
  const r = rows[k];
  const lbl = (chartTab === 'today') ? (r.lbl + ':00–' + r.lbl + ':59') : ('Tarih ' + r.lbl);
  el.innerHTML = `<b>📌 ${lbl}</b> &rarr; <span style="color:#fbbf24">☀️ ${r.pv.toFixed(2)}</span> • <span style="color:#38bdf8">🏠 Ev: ${r.load.toFixed(2)}</span> • <span style="color:#10b981">🔋 +${r.chg.toFixed(2)}</span>/<span style="color:#2dd4bf">-${r.dis.toFixed(2)}</span> • <span style="color:#fb923c">🔌 TEDAŞ: ${r.grid.toFixed(2)} kWh</span>`;
}

function roundTopBar(ctx, x, y, w, h, r){
  const rad = Math.min(r, w / 2, h);
  ctx.beginPath();
  ctx.moveTo(x, y + h);
  ctx.lineTo(x, y + rad);
  ctx.quadraticCurveTo(x, y, x + rad, y);
  ctx.lineTo(x + w - rad, y);
  ctx.quadraticCurveTo(x + w, y, x + w, y + rad);
  ctx.lineTo(x + w, y + h);
  ctx.closePath();
  ctx.fill();
}

function drawChart(){
  const cv = document.getElementById('energyChart');
  if(!cv || !curStats || cv.parentElement.clientWidth < 50) return;
  const dpr = window.devicePixelRatio || 1;
  const W = Math.max(cv.parentElement.clientWidth - 10, 280);
  const H = 220;
  cv.width = W * dpr;
  cv.height = H * dpr;
  cv.style.width = W + 'px';
  cv.style.height = H + 'px';
  const ctx = cv.getContext('2d');
  ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, W, H);

  const padL = 40, padR = 10, padT = 20, padB = 26;
  const cW = W - padL - padR, cH = H - padT - padB;

  const rows = [];
  if(chartTab === 'month'){
    (curStats.days || []).forEach(d => {
      const hasDis = d.length >= 7;
      rows.push({
        lbl:  d[0],
        pv:   Number(d[1]||0),
        load: Number(d[2]||0),
        chg:  Number(d[3]||0),
        dis:  hasDis ? Number(d[4]||0) : 0,
        grid: hasDis ? (Number(d[5]||0) + Number(d[6]||0)) : (Number(d[4]||0) + Number(d[5]||0))
      });
    });
  } else {
    const hp = curStats.hPv||[], hl = curStats.hLoad||[], hc = curStats.hChg||[], hd = curStats.hDis||[], hg = curStats.hGrid||[];
    for(let h=0; h<24; h++){
      rows.push({
        lbl:  (h<10?'0':'')+h,
        pv:   Number(hp[h]||0),
        load: Number(hl[h]||0),
        chg:  Number(hc[h]||0),
        dis:  Number(hd[h]||0),
        grid: Number(hg[h]||0)
      });
    }
  }

  const n = Math.max(rows.length, 1);
  if(selChartIdx < 0 || selChartIdx >= rows.length){
    if(chartTab === 'today' && curStats.hr >= 0 && curStats.hr < 24) selChartIdx = curStats.hr;
    else selChartIdx = rows.length - 1;
  }
  chartGeom = { padL, cW, n, rows };
  updateChartTip(rows, selChartIdx);

  const palette = [
    { key:'pv',   col:'#fbbf24' },
    { key:'load', col:'#38bdf8' },
    { key:'chg',  col:'#10b981' },
    { key:'dis',  col:'#2dd4bf' },
    { key:'grid', col:'#fb923c' }
  ];
  const activeSeries = palette.filter(p => {
    if(!seriesOn[p.key]) return false;
    if(p.key === 'grid' && !rows.some(r => r.grid > 0.005)) return false;
    if((p.key === 'chg' || p.key === 'dis') && chartTab === 'today' && !rows.some(r => r[p.key] > 0.005)) return false;
    return true;
  });
  if(activeSeries.length === 0) activeSeries.push(palette[0]);

  let rawMax = 0;
  rows.forEach(r => activeSeries.forEach(s => { if(r[s.key] > rawMax) rawMax = r[s.key]; }));
  const maxV = niceMax(rawMax);

  ctx.font = '10px system-ui, sans-serif';
  ctx.fillStyle = '#64748b';
  ctx.textAlign = 'left';
  ctx.fillText('kWh', 4, 12);

  ctx.lineWidth = 1;
  ctx.textAlign = 'right';
  for(let g=0; g<=4; g++){
    const y = padT + cH - (cH * g / 4);
    ctx.strokeStyle = (g === 0) ? '#334155' : '#1e293b';
    ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(W - padR, y); ctx.stroke();
    const val = (maxV * g / 4);
    ctx.fillStyle = '#94a3b8';
    ctx.fillText(maxV >= 10 ? val.toFixed(0) : val.toFixed(1), padL - 6, y + 3.5);
  }

  const slotW = cW / n;
  const barCnt = activeSeries.length;
  const gap = (n <= 8) ? 4 : 1.2;
  const maxSingleW = (n <= 6) ? 28 : ((n <= 14) ? 16 : 9);
  const barW = Math.max(2.5, Math.min(maxSingleW, (slotW * 0.72 - gap * (barCnt - 1)) / barCnt));
  const groupW = barW * barCnt + gap * (barCnt - 1);

  for(let i=0; i<rows.length; i++){
    const cx = padL + (i + 0.5) * slotW;
    if(i === selChartIdx){
      ctx.fillStyle = 'rgba(56, 189, 248, 0.10)';
      const hw = Math.min(slotW * 0.92, Math.max(groupW + 12, 20));
      ctx.fillRect(cx - hw / 2, padT, hw, cH);
    }

    const x0 = cx - groupW / 2;
    for(let b=0; b<barCnt; b++){
      const s = activeSeries[b];
      const v = rows[i][s.key] || 0;
      if(v <= 0.001) continue;
      const bh = Math.max(3, (v / maxV) * cH);
      const bx = x0 + b * (barW + gap);
      const by = padT + cH - bh;
      ctx.fillStyle = s.col;
      roundTopBar(ctx, bx, by, barW, bh, 3.5);

      if(chartTab === 'month' && n <= 7 && barW >= 14){
        ctx.fillStyle = '#e2e8f0';
        ctx.font = 'bold 9.5px system-ui, sans-serif';
        ctx.textAlign = 'center';
        ctx.fillText(v >= 10 ? v.toFixed(1) : v.toFixed(2), bx + barW / 2, by - 4);
      }
    }

    const showLbl = (chartTab === 'month')
      ? (n <= 10 || i % Math.ceil(n / 8) === 0 || i === n - 1)
      : (i % 3 === 0 || i === 23);
    if(showLbl){
      const isLiveHr = (chartTab === 'today' && i === curStats.hr);
      ctx.fillStyle = (i === selChartIdx || isLiveHr) ? '#38bdf8' : '#94a3b8';
      ctx.font = (i === selChartIdx || isLiveHr) ? 'bold 10.5px system-ui, sans-serif' : '10px system-ui, sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(rows[i].lbl, cx, H - 8);
    }
  }
}

(function bindChartEvents(){
  const cv = document.getElementById('energyChart');
  if(!cv) return;
  const pick = (clientX) => {
    const rect = cv.getBoundingClientRect();
    const x = clientX - rect.left - chartGeom.padL;
    if(x < 0 || x > chartGeom.cW || chartGeom.n <= 0) return;
    const idx = Math.max(0, Math.min(chartGeom.n - 1, Math.floor((x / chartGeom.cW) * chartGeom.n)));
    if(idx !== selChartIdx){ selChartIdx = idx; drawChart(); }
  };
  cv.addEventListener('click', e => pick(e.clientX));
  cv.addEventListener('mousemove', e => { if(e.buttons === 0) pick(e.clientX); });
  cv.addEventListener('touchstart', e => { if(e.touches[0]) pick(e.touches[0].clientX); }, { passive: true });
})();

window.addEventListener('resize', drawChart);
connectMqtt();
</script>
</body>
</html>

)HTMLEND";

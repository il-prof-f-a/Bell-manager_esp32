#ifndef DEBUG_PAGE_H
#define DEBUG_PAGE_H

#include <pgmspace.h>

// ============================================
// Debug Page v6.0 - Per TM1621 Driver v8.0 (ESPEasy P148)
// ============================================

const char DEBUG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>TM1621 Debug - Bell-Manager</title>
  <style>
    * { box-sizing: border-box; }
    body { margin: 0; font-family: monospace; background: #1a1a2e; color: #eee; padding: 15px; }
    h1 { color: #0f0; margin-bottom: 10px; font-size: 20px; }
    h2 { color: #0ff; margin: 15px 0 8px; border-bottom: 1px solid #333; padding-bottom: 5px; font-size: 14px; }
    h3 { color: #ff0; margin: 10px 0 5px; font-size: 12px; }

    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 15px; }

    .card {
      background: #16213e;
      border: 1px solid #333;
      border-radius: 8px;
      padding: 12px;
    }

    .status-row {
      display: flex;
      justify-content: space-between;
      padding: 5px 0;
      border-bottom: 1px solid #222;
      font-size: 12px;
    }
    .status-label { color: #888; }
    .status-value { color: #0f0; font-weight: bold; }
    .status-value.off { color: #f00; }

    .btn {
      padding: 8px 12px;
      margin: 3px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-family: monospace;
      font-weight: bold;
      font-size: 11px;
    }
    .btn:hover { opacity: 0.8; }
    .btn:active { transform: scale(0.95); }

    .btn-on { background: #0a0; color: #fff; }
    .btn-off { background: #a00; color: #fff; }
    .btn-test { background: #00a; color: #fff; }
    .btn-warn { background: #a50; color: #fff; }
    .btn-info { background: #058; color: #fff; }

    .gpio-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 8px 0;
      border-bottom: 1px solid #222;
    }
    .gpio-name { font-size: 12px; flex: 1; }
    .gpio-led {
      width: 16px; height: 16px;
      border-radius: 50%;
      margin: 0 8px;
    }
    .gpio-led.on { background: #0f0; box-shadow: 0 0 8px #0f0; }
    .gpio-led.off { background: #333; }

    input, select {
      padding: 6px;
      background: #000;
      color: #0f0;
      border: 1px solid #333;
      border-radius: 3px;
      font-family: monospace;
      font-size: 12px;
    }
    input[type="number"] { width: 70px; }
    input[type="text"] { width: 80px; }

    .buf-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 2px;
      margin: 10px 0;
      font-size: 10px;
    }
    .buf-cell {
      background: #000;
      padding: 4px 2px;
      text-align: center;
      border: 1px solid #333;
      color: #0f0;
    }
    .buf-cell.active { background: #030; border-color: #0f0; }
    .buf-cell.header { background: #222; color: #888; font-weight: bold; }

    .test-box {
      background: #300;
      border: 2px solid #f00;
      padding: 10px;
      margin-bottom: 10px;
      border-radius: 5px;
    }
    .test-box.active {
      background: #030;
      border-color: #0f0;
    }

    .info-text { color: #888; font-size: 10px; margin: 5px 0; }

    .display-preview {
      background: #000;
      border: 2px solid #0a0;
      border-radius: 8px;
      padding: 15px;
      text-align: center;
      margin: 10px 0;
    }
    .display-row {
      font-size: 28px;
      font-weight: bold;
      color: #0f0;
      letter-spacing: 8px;
      font-family: 'Courier New', monospace;
    }
    .display-units {
      font-size: 10px;
      color: #0a0;
      margin-top: 2px;
    }

    #log {
      background: #000;
      border: 1px solid #333;
      padding: 8px;
      height: 150px;
      overflow-y: auto;
      font-size: 11px;
      color: #0f0;
    }
    .log-time { color: #666; }
    .log-error { color: #f00; }
    .log-warn { color: #ff0; }

    .flex-row {
      display: flex;
      gap: 5px;
      align-items: center;
      flex-wrap: wrap;
      margin: 5px 0;
    }
  </style>
</head>
<body>
  <h1>// TM1621 Debug Console v6.0 (ESPEasy P148)</h1>
  <div id="lastUpdate" style="color:#888;font-size:11px;margin-bottom:10px;">Last update: --</div>

  <div class="grid">
    <!-- TEST MODE + STATUS -->
    <div class="card">
      <div class="test-box" id="testModeBox">
        <div style="display:flex;justify-content:space-between;align-items:center;">
          <span style="color:#ff0;font-weight:bold;">TEST MODE</span>
          <span id="testModeStatus" style="font-weight:bold;color:#888;">OFF</span>
        </div>
        <p class="info-text">Blocca aggiornamenti auto del display. Si disattiva uscendo dalla pagina.</p>
        <button class="btn btn-on" onclick="setTestMode(true)">ATTIVA TEST</button>
        <button class="btn btn-off" onclick="setTestMode(false)">DISATTIVA</button>
      </div>

      <h2>Display Preview</h2>
      <div class="display-preview">
        <div class="display-row" id="prevRow0">----</div>
        <div class="display-units" id="prevUnits0"></div>
        <hr style="border-color:#030;margin:5px 0;">
        <div class="display-row" id="prevRow1">----</div>
        <div class="display-units" id="prevUnits1"></div>
      </div>

      <h2>TM1621 Status</h2>
      <div class="status-row">
        <span class="status-label">Driver</span>
        <span class="status-value">ESPHome exact v9.0</span>
      </div>
      <div class="status-row">
        <span class="status-label">Initialized</span>
        <span class="status-value" id="dispInit">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">LCD On</span>
        <span class="status-value" id="dispOn">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Row 0</span>
        <span class="status-value" id="dispRow0">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Row 1</span>
        <span class="status-value" id="dispRow1">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Units</span>
        <span class="status-value" id="dispUnits">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Write Count</span>
        <span class="status-value" id="dispWrites">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Cmd Count</span>
        <span class="status-value" id="dispCmds">--</span>
      </div>

      <h3>Pixel Buffer (8 bytes @ 0x10)</h3>
      <div id="bufGrid" class="buf-grid"></div>
    </div>

    <!-- WRITE TEXT -->
    <div class="card">
      <h2>Write Text (writeString)</h2>
      <p class="info-text">Scrive testo su una riga (max 4 caratteri). Supporta: 0-9, A-Z, -, ?</p>

      <h3>Riga 1 (top)</h3>
      <div class="flex-row">
        <input type="text" id="writeRow0" value="----" maxlength="11">
        <button class="btn btn-test" onclick="writeRow(0)">WRITE</button>
      </div>

      <h3>Riga 2 (bottom)</h3>
      <div class="flex-row">
        <input type="text" id="writeRow1" value="----" maxlength="11">
        <button class="btn btn-test" onclick="writeRow(1)">WRITE</button>
      </div>

      <h3>Scrivi Entrambe (writeStrings)</h3>
      <div class="flex-row">
        <span>R1:</span>
        <input type="text" id="writeBoth0" value="HELo" maxlength="11" style="width:60px;">
        <span>R2:</span>
        <input type="text" id="writeBoth1" value="1234" maxlength="11" style="width:60px;">
        <button class="btn btn-test" onclick="writeBoth()">WRITE</button>
      </div>

      <h3>Quick Text</h3>
      <div>
        <button class="btn btn-info" onclick="writeQuick('----','----')">----</button>
        <button class="btn btn-info" onclick="writeQuick('    ','    ')">Clear</button>
        <button class="btn btn-info" onclick="writeQuick('8888','8888')">8888</button>
        <button class="btn btn-info" onclick="writeQuick('1234','5678')">1234/5678</button>
        <button class="btn btn-info" onclick="writeQuick('HELo','    ')">HELo</button>
        <button class="btn btn-info" onclick="writeQuick('bELL','    ')">bELL</button>
        <button class="btn btn-info" onclick="writeQuick('Conn','    ')">Conn</button>
        <button class="btn btn-info" onclick="writeQuick('ABCD','EFGH')">ABCD/EFGH</button>
      </div>

      <h2>Write Float (writeFloat)</h2>
      <p class="info-text">Scrive un numero con decimale. Es: 23.5 -> "23.5"</p>

      <h3>Float Riga 1</h3>
      <div class="flex-row">
        <input type="number" id="float0" value="23.5" step="0.1">
        <button class="btn btn-test" onclick="writeFloat(0)">WRITE</button>
      </div>

      <h3>Float Riga 2</h3>
      <div class="flex-row">
        <input type="number" id="float1" value="45.8" step="0.1">
        <button class="btn btn-test" onclick="writeFloat(1)">WRITE</button>
      </div>

      <h3>Due Float (writeFloats)</h3>
      <div class="flex-row">
        <span>V1:</span>
        <input type="number" id="floatBoth0" value="220.5" step="0.1" style="width:70px;">
        <span>V2:</span>
        <input type="number" id="floatBoth1" value="1.2" step="0.1" style="width:70px;">
        <button class="btn btn-test" onclick="writeFloatBoth()">WRITE</button>
      </div>
    </div>

    <!-- UNIT SYMBOLS -->
    <div class="card">
      <h2>Unit Symbols (setUnit)</h2>
      <p class="info-text">Attiva/disattiva i simboli delle unita' di misura sul display</p>

      <h3>Riga 1 (top) - Celsius / Fahrenheit / V / kWh</h3>
      <div>
        <button class="btn btn-info" onclick="setUnit('celsius')">Celsius</button>
        <button class="btn btn-info" onclick="setUnit('fahrenheit')">Fahrenheit</button>
        <button class="btn btn-info" onclick="setUnit('volt_amp')">V/A</button>
        <button class="btn btn-info" onclick="setUnit('kwh_watt')">kWh/W</button>
        <button class="btn btn-off" onclick="setUnit('none_top')">None (top)</button>
      </div>

      <h3>Riga 2 (bottom) - %RH / A / W</h3>
      <div>
        <button class="btn btn-info" onclick="setUnit('humidity')">%RH</button>
        <button class="btn btn-info" onclick="setUnit('volt_amp_bot')">V/A</button>
        <button class="btn btn-info" onclick="setUnit('kwh_watt_bot')">kWh/W</button>
        <button class="btn btn-off" onclick="setUnit('none_bot')">None (bot)</button>
      </div>

      <h3>Preset Combinati</h3>
      <div>
        <button class="btn btn-warn" onclick="setUnit('clear_all')">Clear All Units</button>
        <button class="btn btn-info" onclick="presetTemp()">Temp (C + %RH)</button>
        <button class="btn btn-info" onclick="presetEnergy()">Energy (kWh + W)</button>
        <button class="btn btn-info" onclick="presetPower()">Power (V + A)</button>
      </div>

      <h2>Raw Data (writeRawData)</h2>
      <p class="info-text">Scrive 8 byte raw direttamente al pixel buffer (hex, 16 chars)</p>
      <div class="flex-row">
        <input type="text" id="rawData" value="FFFFFFFFFFFFFFFF" maxlength="16" style="width:150px;">
        <button class="btn btn-warn" onclick="writeRaw()">WRITE RAW</button>
      </div>
      <div style="margin-top:5px;">
        <button class="btn btn-info" onclick="setRawPreset('FFFFFFFFFFFFFFFF')" style="font-size:9px;">All ON</button>
        <button class="btn btn-info" onclick="setRawPreset('0000000000000000')" style="font-size:9px;">All OFF</button>
        <button class="btn btn-info" onclick="setRawPreset('FF000000000000FF')" style="font-size:9px;">Corners</button>
        <button class="btn btn-info" onclick="setRawPreset('00FF00FF00FF00FF')" style="font-size:9px;">Alternate</button>
      </div>

      <h2>Display Control</h2>
      <h3>Power</h3>
      <div>
        <button class="btn btn-on" onclick="dispCmd('lcd_on')">LCD ON</button>
        <button class="btn btn-off" onclick="dispCmd('lcd_off')">LCD OFF</button>
        <button class="btn btn-warn" onclick="dispCmd('reinit')">Re-Init</button>
      </div>

      <h3>Segment Test</h3>
      <div>
        <button class="btn btn-test" onclick="dispCmd('all_on')">All ON</button>
        <button class="btn btn-test" onclick="dispCmd('all_off')">All OFF</button>
      </div>
    </div>

    <!-- GPIO CONTROL -->
    <div class="card">
      <h2>GPIO Control</h2>

      <div class="gpio-row">
        <span class="gpio-name">Button (GPIO0)</span>
        <span class="gpio-led" id="btnLed"></span>
        <span id="btnText">--</span>
      </div>

      <div class="gpio-row">
        <span class="gpio-name">Relay (GPIO13)</span>
        <span class="gpio-led" id="relayLed"></span>
        <div>
          <button class="btn btn-on" onclick="setGpio('relay',true)">ON</button>
          <button class="btn btn-off" onclick="setGpio('relay',false)">OFF</button>
        </div>
      </div>

      <div class="gpio-row">
        <span class="gpio-name">LED WiFi (GPIO5)</span>
        <span class="gpio-led" id="ledWifiLed"></span>
        <div>
          <button class="btn btn-on" onclick="setGpio('ledWifi',true)">ON</button>
          <button class="btn btn-off" onclick="setGpio('ledWifi',false)">OFF</button>
        </div>
      </div>

      <div class="gpio-row">
        <span class="gpio-name">LED Relay (GPIO18)</span>
        <span class="gpio-led" id="ledRelayLed"></span>
        <div>
          <button class="btn btn-on" onclick="setGpio('ledRelay',true)">ON</button>
          <button class="btn btn-off" onclick="setGpio('ledRelay',false)">OFF</button>
        </div>
      </div>

      <h2>System Info</h2>
      <div class="status-row">
        <span class="status-label">Version</span>
        <span class="status-value" id="version">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Uptime</span>
        <span class="status-value" id="uptime">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Free Heap</span>
        <span class="status-value" id="freeHeap">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">WiFi</span>
        <span class="status-value" id="wifiState">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">IP</span>
        <span class="status-value" id="wifiIP">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">NTP Synced</span>
        <span class="status-value" id="ntpSynced">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Time</span>
        <span class="status-value" id="currentTime">--</span>
      </div>

      <h2>TM1621 Hardware (POWR316D)</h2>
      <div class="status-row"><span class="status-label">DA (Data)</span><span class="status-value">GPIO14</span></div>
      <div class="status-row"><span class="status-label">CS</span><span class="status-value">GPIO25</span></div>
      <div class="status-row"><span class="status-label">RD</span><span class="status-value">GPIO26</span></div>
      <div class="status-row"><span class="status-label">WR (Clock)</span><span class="status-value">GPIO27</span></div>
      <div class="status-row"><span class="status-label">BIAS</span><span class="status-value">0x29 (1/3, 4COM)</span></div>
      <div class="status-row"><span class="status-label">Pulse</span><span class="status-value">10 us</span></div>
      <div class="status-row"><span class="status-label">Buffer addr</span><span class="status-value">0x10 (8 bytes)</span></div>
    </div>
  </div>

  <!-- ACTIONS -->
  <div class="card" style="margin-top:15px;">
    <h2>Actions</h2>
    <button class="btn btn-warn" onclick="restart()">RESTART DEVICE</button>
    <a href="/" class="btn btn-info" style="text-decoration:none;display:inline-block;">Back to Main</a>
    <button class="btn btn-test" onclick="loadStatus()">Refresh Status</button>
    <button class="btn btn-info" onclick="dispCmd('print_status')">Print to Serial</button>
  </div>

  <!-- LOG -->
  <div class="card" style="margin-top:15px;">
    <h2>Activity Log</h2>
    <div id="log"></div>
  </div>

  <script>
    function log(msg, type = 'info') {
      const el = document.getElementById('log');
      const time = new Date().toLocaleTimeString();
      const cls = type === 'error' ? 'log-error' : (type === 'warn' ? 'log-warn' : '');
      el.innerHTML = `<div class="${cls}"><span class="log-time">[${time}]</span> ${msg}</div>` + el.innerHTML;
      if (el.children.length > 50) el.removeChild(el.lastChild);
    }

    async function api(url, options = {}) {
      try {
        const res = await fetch(url, {
          ...options,
          headers: { 'Content-Type': 'application/json' }
        });
        return await res.json();
      } catch (e) {
        log('Error: ' + e.message, 'error');
        return null;
      }
    }

    // === Write Text ===
    async function writeRow(row) {
      const text = document.getElementById('writeRow' + row).value;
      log(`writeString row${row}: "${text}"`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_row', row, text })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function writeBoth() {
      const str1 = document.getElementById('writeBoth0').value;
      const str2 = document.getElementById('writeBoth1').value;
      log(`writeStrings: "${str1}" / "${str2}"`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_both', str1, str2 })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function writeQuick(s1, s2) {
      log(`Quick: "${s1}" / "${s2}"`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_both', str1: s1, str2: s2 })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    // === Write Float ===
    async function writeFloat(row) {
      const value = parseFloat(document.getElementById('float' + row).value) || 0;
      log(`writeFloat row${row}: ${value}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_float', row, value })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function writeFloatBoth() {
      const v1 = parseFloat(document.getElementById('floatBoth0').value) || 0;
      const v2 = parseFloat(document.getElementById('floatBoth1').value) || 0;
      log(`writeFloats: ${v1} / ${v2}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_floats', value1: v1, value2: v2 })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    // === Unit Symbols ===
    async function setUnit(unit) {
      log(`setUnit: ${unit}`, 'warn');
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function presetTemp() {
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit: 'celsius' })
      });
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit: 'humidity' })
      });
      log('Preset: Celsius + %RH');
      setTimeout(loadStatus, 300);
    }

    async function presetEnergy() {
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit: 'kwh_watt' })
      });
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit: 'kwh_watt_bot' })
      });
      log('Preset: kWh + W');
      setTimeout(loadStatus, 300);
    }

    async function presetPower() {
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit: 'volt_amp' })
      });
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_unit', unit: 'volt_amp_bot' })
      });
      log('Preset: V + A');
      setTimeout(loadStatus, 300);
    }

    // === Raw Data ===
    async function writeRaw() {
      const hex = document.getElementById('rawData').value;
      log(`writeRawData: 0x${hex}`, 'warn');
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_raw', hex })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    function setRawPreset(hex) {
      document.getElementById('rawData').value = hex;
    }

    // === Display Commands ===
    async function dispCmd(cmd) {
      log(`Command: ${cmd}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: cmd })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    // === Test Mode ===
    async function setTestMode(enable) {
      log(`Test mode: ${enable ? 'ON' : 'OFF'}`, 'warn');
      const r = await api(`/api/display/test_mode?enable=${enable ? 1 : 0}`);
      if (r && r.success) {
        updateTestModeUI(enable);
        log(r.message);
      }
    }

    function updateTestModeUI(enabled) {
      const box = document.getElementById('testModeBox');
      const status = document.getElementById('testModeStatus');
      if (enabled) {
        box.className = 'test-box active';
        status.textContent = 'ATTIVO';
        status.style.color = '#0f0';
      } else {
        box.className = 'test-box';
        status.textContent = 'OFF';
        status.style.color = '#888';
      }
    }

    // === GPIO ===
    async function setGpio(gpio, state) {
      log(`GPIO ${gpio}: ${state ? 'ON' : 'OFF'}`);
      await api('/api/debug/gpio', {
        method: 'POST',
        body: JSON.stringify({ gpio, state })
      });
      setTimeout(loadStatus, 300);
    }

    // === Status ===
    async function loadStatus() {
      const d = await api('/api/debug/status');
      if (!d) return;

      // TM1621 Status
      setBool('dispInit', d.dispInit);
      setBool('dispOn', d.dispOn);
      document.getElementById('dispRow0').textContent = d.dispRow0 || '--';
      document.getElementById('dispRow1').textContent = d.dispRow1 || '--';
      document.getElementById('dispUnits').textContent = d.dispUnits || 'none';
      document.getElementById('dispWrites').textContent = d.dispWrites || '0';
      document.getElementById('dispCmds').textContent = d.dispCmds || '0';
      updateTestModeUI(d.testMode);

      // Display Preview
      document.getElementById('prevRow0').textContent = (d.dispRow0 || '----').substring(0, 4);
      document.getElementById('prevRow1').textContent = (d.dispRow1 || '----').substring(0, 4);

      // Units preview
      let u0 = '', u1 = '';
      const units = d.dispUnits || '';
      if (units.indexOf('C') >= 0) u0 += 'C ';
      if (units.indexOf('F') >= 0) u0 += 'F ';
      if (units.indexOf('V/A') >= 0) { u0 += 'V'; u1 += 'A'; }
      if (units.indexOf('kWh/W') >= 0) { u0 += 'kWh'; u1 += 'W'; }
      if (units.indexOf('%RH') >= 0) u1 += '%RH';
      document.getElementById('prevUnits0').textContent = u0;
      document.getElementById('prevUnits1').textContent = u1;

      // Buffer Grid
      updateBufGrid(d.dispBuffer || '');

      // GPIO
      setLed('btnLed', d.button);
      document.getElementById('btnText').textContent = d.button ? 'PRESSED' : 'Released';
      setLed('relayLed', d.relay);
      setLed('ledWifiLed', d.ledWifi);
      setLed('ledRelayLed', d.ledRelay);

      // System
      document.getElementById('version').textContent = d.version || '--';
      document.getElementById('uptime').textContent = formatUptime(d.uptime || 0);
      document.getElementById('freeHeap').textContent = formatBytes(d.freeHeap || 0);
      document.getElementById('wifiState').textContent = d.wifiStateName || '--';
      document.getElementById('wifiIP').textContent = d.wifiIP || '--';
      setBool('ntpSynced', d.ntpSynced);
      document.getElementById('currentTime').textContent = d.currentTime || '--';

      document.getElementById('lastUpdate').textContent = 'Last update: ' + new Date().toLocaleTimeString();
    }

    function updateBufGrid(bufStr) {
      const grid = document.getElementById('bufGrid');
      const hex = bufStr.replace(/ /g, '');
      let html = '';
      // Header
      html += '<div class="buf-cell header">Idx</div>';
      html += '<div class="buf-cell header">Hex</div>';
      html += '<div class="buf-cell header">Bin</div>';
      html += '<div class="buf-cell header">Row</div>';
      const rowLabels = ['R1-D0','R1-D1','R1-D2','R1-D3','R2-D3','R2-D2','R2-D1','R2-D0'];
      for (let i = 0; i < 8; i++) {
        const val = hex.substring(i*2, i*2+2) || '00';
        const num = parseInt(val, 16);
        const active = num !== 0 ? 'active' : '';
        const bin = num.toString(2).padStart(8, '0');
        html += `<div class="buf-cell ${active}">${i}</div>`;
        html += `<div class="buf-cell ${active}">0x${val}</div>`;
        html += `<div class="buf-cell ${active}">${bin}</div>`;
        html += `<div class="buf-cell ${active}">${rowLabels[i]}</div>`;
      }
      grid.innerHTML = html;
    }

    function setBool(id, val) {
      const el = document.getElementById(id);
      el.textContent = val ? 'YES' : 'NO';
      el.className = 'status-value ' + (val ? '' : 'off');
    }

    function setLed(id, on) {
      document.getElementById(id).className = 'gpio-led ' + (on ? 'on' : 'off');
    }

    function formatUptime(sec) {
      const h = Math.floor(sec / 3600);
      const m = Math.floor((sec % 3600) / 60);
      const s = sec % 60;
      return `${h}h ${m}m ${s}s`;
    }

    function formatBytes(b) {
      if (b < 1024) return b + ' B';
      return (b / 1024).toFixed(1) + ' KB';
    }

    async function restart() {
      if (!confirm('Riavviare il dispositivo?')) return;
      log('Restarting...', 'warn');
      await api('/api/debug/restart', { method: 'POST' });
    }

    // Disattiva test mode quando si lascia la pagina
    window.addEventListener('beforeunload', () => {
      navigator.sendBeacon('/api/display/test_mode?enable=0');
    });

    // Init
    log('TM1621 Debug Console v6.0 (ESPEasy P148)');
    loadStatus();
    setInterval(loadStatus, 5000);
  </script>
</body>
</html>
)rawliteral";

#endif // DEBUG_PAGE_H

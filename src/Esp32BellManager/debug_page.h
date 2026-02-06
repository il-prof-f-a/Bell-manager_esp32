#ifndef DEBUG_PAGE_H
#define DEBUG_PAGE_H

#include <pgmspace.h>

// ============================================
// Debug Page v3.0 - Per TM1621 Driver v4.0
// Interfaccia di test basata sul datasheet ufficiale
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
    input[type="number"] { width: 60px; }

    .ram-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 2px;
      margin: 10px 0;
      font-size: 9px;
    }
    .ram-cell {
      background: #000;
      padding: 3px 2px;
      text-align: center;
      border: 1px solid #333;
      color: #0f0;
    }
    .ram-cell.active { background: #030; border-color: #0f0; }

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
    .mono { font-family: monospace; }

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
  </style>
</head>
<body>
  <h1>// TM1621 Debug Console v3.0</h1>
  <div id="lastUpdate" style="color:#888;font-size:11px;margin-bottom:10px;">Last update: --</div>

  <div class="grid">
    <!-- TEST MODE BOX -->
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

      <h2>TM1621 Status</h2>
      <div class="status-row">
        <span class="status-label">Initialized</span>
        <span class="status-value" id="dispInit">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">LCD On</span>
        <span class="status-value" id="dispOn">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">BIAS</span>
        <span class="status-value" id="dispBias">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Pulse Width</span>
        <span class="status-value" id="dispPulse">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Write Count</span>
        <span class="status-value" id="dispWrites">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Cmd Count</span>
        <span class="status-value" id="dispCmds">--</span>
      </div>

      <h3>RAM Mirror (32 x 4 bit)</h3>
      <div id="ramGrid" class="ram-grid"></div>
    </div>

    <!-- DISPLAY CONTROL -->
    <div class="card">
      <h2>Display Control</h2>

      <h3>Power</h3>
      <div>
        <button class="btn btn-on" onclick="testCmd('lcd_on')">LCD ON</button>
        <button class="btn btn-off" onclick="testCmd('lcd_off')">LCD OFF</button>
        <button class="btn btn-warn" onclick="testCmd('reinit')">Re-Init</button>
      </div>

      <h3>Segment Test</h3>
      <div>
        <button class="btn btn-test" onclick="testCmd('all_on')">All ON (0xFF)</button>
        <button class="btn btn-test" onclick="testCmd('all_off')">All OFF</button>
      </div>

      <h3>Test Singoli Byte (0xFF)</h3>
      <p class="info-text">Premi CLEAR prima, poi un byte alla volta per vedere quale digit si accende</p>
      <div style="display:flex;flex-wrap:wrap;gap:2px;margin-bottom:5px;">
        <button class="btn btn-off" onclick="testCmd('clear_all')" style="padding:4px 8px;">CLEAR</button>
      </div>
      <div style="display:flex;flex-wrap:wrap;gap:2px;">
        <button class="btn btn-info" onclick="testCmd('b0')" style="padding:4px 6px;font-size:10px;">0</button>
        <button class="btn btn-info" onclick="testCmd('b1')" style="padding:4px 6px;font-size:10px;">1</button>
        <button class="btn btn-info" onclick="testCmd('b2')" style="padding:4px 6px;font-size:10px;">2</button>
        <button class="btn btn-info" onclick="testCmd('b3')" style="padding:4px 6px;font-size:10px;">3</button>
        <button class="btn btn-info" onclick="testCmd('b4')" style="padding:4px 6px;font-size:10px;">4</button>
        <button class="btn btn-info" onclick="testCmd('b5')" style="padding:4px 6px;font-size:10px;">5</button>
        <button class="btn btn-info" onclick="testCmd('b6')" style="padding:4px 6px;font-size:10px;">6</button>
        <button class="btn btn-info" onclick="testCmd('b7')" style="padding:4px 6px;font-size:10px;">7</button>
        <button class="btn btn-warn" onclick="testCmd('b8')" style="padding:4px 6px;font-size:10px;">8</button>
        <button class="btn btn-warn" onclick="testCmd('b9')" style="padding:4px 6px;font-size:10px;">9</button>
        <button class="btn btn-warn" onclick="testCmd('b10')" style="padding:4px 6px;font-size:10px;">10</button>
        <button class="btn btn-warn" onclick="testCmd('b11')" style="padding:4px 6px;font-size:10px;">11</button>
        <button class="btn btn-warn" onclick="testCmd('b12')" style="padding:4px 6px;font-size:10px;">12</button>
        <button class="btn btn-warn" onclick="testCmd('b13')" style="padding:4px 6px;font-size:10px;">13</button>
        <button class="btn btn-warn" onclick="testCmd('b14')" style="padding:4px 6px;font-size:10px;">14</button>
        <button class="btn btn-warn" onclick="testCmd('b15')" style="padding:4px 6px;font-size:10px;">15</button>
      </div>

      <h3>Fill Personalizzato</h3>
      <div style="display:flex;gap:5px;align-items:center;flex-wrap:wrap;">
        <span>Addr:</span>
        <input type="number" id="fillAddr" value="0" min="0" max="31" style="width:50px;">
        <span>Count:</span>
        <input type="number" id="fillCount" value="8" min="1" max="32" style="width:50px;">
        <span>Val:</span>
        <select id="fillVal">
          <option value="255">0xFF</option>
          <option value="0">0x00</option>
          <option value="170">0xAA</option>
          <option value="85">0x55</option>
          <option value="15">0x0F</option>
          <option value="240">0xF0</option>
        </select>
        <button class="btn btn-warn" onclick="fillAt()">FILL</button>
      </div>

      <h3>Scrivi Singolo Nibble (4 bit)</h3>
      <p class="info-text">RAM = 32 indirizzi, ogni addr = 4 bit (D0-D3)</p>
      <div style="display:flex;gap:5px;align-items:center;flex-wrap:wrap;">
        <span>Addr:</span>
        <input type="number" id="nibbleAddr" value="0" min="0" max="31">
        <span>Data:</span>
        <select id="nibbleData">
          <option value="15">0xF (1111)</option>
          <option value="0">0x0 (0000)</option>
          <option value="1">0x1 (0001)</option>
          <option value="2">0x2 (0010)</option>
          <option value="4">0x4 (0100)</option>
          <option value="8">0x8 (1000)</option>
          <option value="5">0x5 (0101)</option>
          <option value="10">0xA (1010)</option>
          <option value="7">0x7 (0111)</option>
          <option value="14">0xE (1110)</option>
        </select>
        <button class="btn btn-warn" onclick="writeNibble()">WRITE</button>
      </div>

      <h3>Scrivi Multi-Nibble</h3>
      <p class="info-text">Scrive nibble consecutivi con auto-increment</p>
      <div style="display:flex;gap:5px;align-items:center;flex-wrap:wrap;">
        <span>Addr:</span>
        <input type="number" id="multiAddr" value="0" min="0" max="31">
        <span>Count:</span>
        <input type="number" id="multiCount" value="4" min="1" max="32">
        <span>Val:</span>
        <select id="multiValue">
          <option value="15">0xF</option>
          <option value="0">0x0</option>
          <option value="5">0x5</option>
          <option value="10">0xA</option>
        </select>
        <button class="btn btn-warn" onclick="writeMulti()">WRITE</button>
      </div>

      <h3>Fill RAM</h3>
      <div style="display:flex;gap:5px;align-items:center;">
        <span>Value:</span>
        <select id="fillValue">
          <option value="15">0xF (all ON)</option>
          <option value="0">0x0 (all OFF)</option>
          <option value="5">0x5 (0101)</option>
          <option value="10">0xA (1010)</option>
        </select>
        <button class="btn btn-test" onclick="fillRam()">FILL ALL</button>
      </div>
    </div>

    <!-- COMMANDS -->
    <div class="card">
      <h2>Send Command</h2>
      <p class="info-text">Invia comando diretto al TM1621 (9 bit MSB first)</p>

      <h3>System</h3>
      <div>
        <button class="btn btn-info" onclick="sendCmd(0x01)">SYS_EN (0x01)</button>
        <button class="btn btn-info" onclick="sendCmd(0x00)">SYS_DIS (0x00)</button>
        <button class="btn btn-info" onclick="sendCmd(0x03)">LCD_ON (0x03)</button>
        <button class="btn btn-info" onclick="sendCmd(0x02)">LCD_OFF (0x02)</button>
      </div>

      <h3>BIAS/COM Config</h3>
      <p class="info-text">POWR316D: 0x24 funziona meglio!</p>
      <div>
        <button class="btn btn-on" onclick="sendCmd(0x24)">0x24 (1/2, 3COM) OK!</button>
        <button class="btn btn-info" onclick="sendCmd(0x25)">0x25 (1/3, 3COM)</button>
        <button class="btn btn-info" onclick="sendCmd(0x28)">0x28 (1/2, 4COM)</button>
        <button class="btn btn-info" onclick="sendCmd(0x29)">0x29 (1/3, 4COM)</button>
      </div>

      <h3>Clock Source</h3>
      <div>
        <button class="btn btn-info" onclick="sendCmd(0x18)">RC 256K (default)</button>
        <button class="btn btn-info" onclick="sendCmd(0x14)">XTAL 32K</button>
      </div>

      <h3>Command Personalizzato</h3>
      <div style="display:flex;gap:5px;align-items:center;">
        <span>Cmd 0x</span>
        <input type="text" id="customCmd" value="03" style="width:40px;">
        <button class="btn btn-warn" onclick="sendCustomCmd()">SEND</button>
      </div>
    </div>

    <!-- CONFIGURATION -->
    <div class="card">
      <h2>Configuration</h2>

      <h3>Pulse Width (us)</h3>
      <p class="info-text">Timing del clock seriale. Default: 5us.</p>
      <div style="display:flex;gap:5px;align-items:center;">
        <select id="pulseWidth">
          <option value="1">1 us (fast)</option>
          <option value="2">2 us</option>
          <option value="5" selected>5 us (default)</option>
          <option value="10">10 us</option>
          <option value="20">20 us (slow)</option>
        </select>
        <button class="btn btn-info" onclick="setPulse()">SET</button>
      </div>

      <h3>BIAS Setting</h3>
      <p class="info-text">Configura e invia comando BIAS</p>
      <div style="display:flex;gap:5px;align-items:center;">
        <select id="biasSelect">
          <option value="40">0x28 - 1/2 bias, 4 COM</option>
          <option value="41">0x29 - 1/3 bias, 4 COM</option>
          <option value="36">0x24 - 1/2 bias, 3 COM</option>
          <option value="37">0x25 - 1/3 bias, 3 COM</option>
        </select>
        <button class="btn btn-info" onclick="setBias()">SET</button>
      </div>

      <h3>Pin Test</h3>
      <p class="info-text">Testa i pin fisici del display</p>
      <div>
        <button class="btn btn-warn" onclick="testCmd('test_pins')">Test Sequenza Pin</button>
        <button class="btn btn-info" onclick="testCmd('print_status')">Print Status (Serial)</button>
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
    </div>

    <!-- SYSTEM INFO -->
    <div class="card">
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
    </div>

    <!-- HARDWARE INFO -->
    <div class="card">
      <h2>TM1621 Hardware</h2>
      <div class="status-row">
        <span class="status-label">CS</span>
        <span class="status-value">GPIO25</span>
      </div>
      <div class="status-row">
        <span class="status-label">WR</span>
        <span class="status-value">GPIO27</span>
      </div>
      <div class="status-row">
        <span class="status-label">DATA</span>
        <span class="status-value">GPIO14</span>
      </div>
      <div class="status-row">
        <span class="status-label">RD</span>
        <span class="status-value">GPIO26</span>
      </div>
      <div class="info-text" style="margin-top:10px;">
        <strong>TM1621 Protocol (datasheet):</strong><br>
        - RAM: 32 x 4 bit<br>
        - WRITE: 101 + Addr(6bit MSB) + Data(4bit LSB)<br>
        - CMD: 100 + Cmd(8bit MSB) + X<br>
        - Data sampled on WR rising edge
      </div>
    </div>
  </div>

  <!-- ACTIONS -->
  <div class="card" style="margin-top:15px;">
    <h2>Actions</h2>
    <button class="btn btn-warn" onclick="restart()">RESTART DEVICE</button>
    <a href="/" class="btn btn-info" style="text-decoration:none;display:inline-block;">Back to Main</a>
    <button class="btn btn-test" onclick="loadStatus()">Refresh Status</button>
  </div>

  <!-- LOG -->
  <div class="card" style="margin-top:15px;">
    <h2>Activity Log</h2>
    <div id="log"></div>
  </div>

  <script>
    // ============================================
    // TM1621 Debug Console v3.0
    // ============================================

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

    // === Display Tests ===
    async function testCmd(test) {
      log(`Test: ${test}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function testPattern(val) {
      log(`Pattern: 0x${val.toString(16)}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'pattern', value: val })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function writeNibble() {
      const addr = parseInt(document.getElementById('nibbleAddr').value) || 0;
      const data = parseInt(document.getElementById('nibbleData').value) || 0;
      log(`Write nibble: addr=${addr} data=0x${data.toString(16)}`, 'warn');
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_nibble', addr, data })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function writeMulti() {
      const addr = parseInt(document.getElementById('multiAddr').value) || 0;
      const count = parseInt(document.getElementById('multiCount').value) || 4;
      const value = parseInt(document.getElementById('multiValue').value) || 0xF;
      log(`Write multi: addr=${addr} count=${count} val=0x${value.toString(16)}`, 'warn');
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'write_multi', addr, count, value })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function fillRam() {
      const value = parseInt(document.getElementById('fillValue').value) || 0xFF;
      log(`Fill RAM: 0x${value.toString(16)}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'fill', value })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function fillAt() {
      const addr = parseInt(document.getElementById('fillAddr').value) || 0;
      const count = parseInt(document.getElementById('fillCount').value) || 8;
      const value = parseInt(document.getElementById('fillVal').value) || 0xFF;
      log(`Fill: ${count}x 0x${value.toString(16).toUpperCase()} @ addr ${addr}`, 'warn');
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'fill_at', addr, count, value })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function sendCmd(cmd) {
      log(`Send cmd: 0x${cmd.toString(16).toUpperCase()}`, 'warn');
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'cmd', cmd })
      });
      if (r && r.message) log(r.message);
    }

    async function sendCustomCmd() {
      const cmd = parseInt(document.getElementById('customCmd').value, 16) || 0;
      await sendCmd(cmd);
    }

    async function setPulse() {
      const value = parseInt(document.getElementById('pulseWidth').value) || 5;
      log(`Set pulse: ${value} us`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_pulse', value })
      });
      if (r && r.message) log(r.message);
      setTimeout(loadStatus, 300);
    }

    async function setBias() {
      const value = parseInt(document.getElementById('biasSelect').value) || 0x28;
      log(`Set bias: 0x${value.toString(16).toUpperCase()}`);
      const r = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'set_bias', value })
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
      document.getElementById('dispBias').textContent = d.dispBias || '--';
      document.getElementById('dispPulse').textContent = (d.dispPulse || '--') + ' us';
      document.getElementById('dispWrites').textContent = d.dispWrites || '0';
      document.getElementById('dispCmds').textContent = d.dispCmds || '0';
      updateTestModeUI(d.testMode);

      // RAM Grid
      updateRamGrid(d.dispRam || '');

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

    function updateRamGrid(ramStr) {
      const grid = document.getElementById('ramGrid');
      // Parse RAM string: "XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX" (16 bytes = 32 hex chars)
      const hex = ramStr.replace(/ /g, '');
      let html = '';
      // Mostra 16 byte (32 caratteri hex, 2 per byte)
      for (let i = 0; i < 16; i++) {
        const val = hex.substring(i*2, i*2+2) || '00';
        const active = val !== '00' ? 'active' : '';
        html += `<div class="ram-cell ${active}" title="Addr ${i}">0x${val}</div>`;
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
    log('TM1621 Debug Console v3.0');
    log('Driver basato su datasheet ufficiale');
    loadStatus();
    setInterval(loadStatus, 5000);
  </script>
</body>
</html>
)rawliteral";

#endif // DEBUG_PAGE_H

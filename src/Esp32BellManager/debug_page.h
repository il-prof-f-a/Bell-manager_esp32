#ifndef DEBUG_PAGE_H
#define DEBUG_PAGE_H

#include <pgmspace.h>

const char DEBUG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Debug - Bell-Manager</title>
  <style>
    * { box-sizing: border-box; }
    body { margin: 0; font-family: monospace; background: #1a1a2e; color: #eee; padding: 20px; }
    h1 { color: #0f0; margin-bottom: 20px; }
    h2 { color: #0ff; margin: 20px 0 10px; border-bottom: 1px solid #333; padding-bottom: 5px; }
    h3 { color: #ff0; margin: 15px 0 8px; font-size: 14px; }

    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }

    .card {
      background: #16213e;
      border: 1px solid #333;
      border-radius: 8px;
      padding: 15px;
    }

    .status-row {
      display: flex;
      justify-content: space-between;
      padding: 8px 0;
      border-bottom: 1px solid #333;
    }
    .status-label { color: #888; }
    .status-value { color: #0f0; font-weight: bold; }
    .status-value.off { color: #f00; }
    .status-value.warn { color: #ff0; }

    .btn {
      padding: 10px 20px;
      margin: 5px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-family: monospace;
      font-weight: bold;
      transition: 0.2s;
    }
    .btn:hover { transform: scale(1.05); }
    .btn:active { transform: scale(0.95); }
    .btn-sm { padding: 5px 10px; font-size: 12px; }

    .btn-on { background: #0a0; color: #fff; }
    .btn-off { background: #a00; color: #fff; }
    .btn-test { background: #00a; color: #fff; }
    .btn-warn { background: #a50; color: #fff; }
    .btn-diag { background: #505; color: #fff; }

    .gpio-control {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 0;
      border-bottom: 1px solid #333;
    }
    .gpio-name { font-size: 14px; }
    .gpio-state {
      width: 20px; height: 20px;
      border-radius: 50%;
      margin: 0 10px;
    }
    .gpio-state.on { background: #0f0; box-shadow: 0 0 10px #0f0; }
    .gpio-state.off { background: #333; }

    .log {
      background: #000;
      border: 1px solid #333;
      padding: 10px;
      height: 200px;
      overflow-y: auto;
      font-size: 12px;
      color: #0f0;
    }
    .log-entry { margin: 2px 0; }
    .log-time { color: #888; }
    .log-error { color: #f00; }
    .log-warn { color: #ff0; }

    #lastUpdate { color: #888; font-size: 12px; }

    .pin-row {
      display: flex;
      align-items: center;
      padding: 5px 0;
      border-bottom: 1px solid #222;
    }
    .pin-label { width: 100px; color: #888; }
    .pin-gpio { width: 60px; color: #0ff; }
  </style>
</head>
<body>
  <h1>// DEBUG CONSOLE</h1>
  <div id="lastUpdate">Last update: --</div>

  <div class="grid">
    <!-- GPIO Status & Control -->
    <div class="card">
      <h2>GPIO Control</h2>

      <div class="gpio-control">
        <span class="gpio-name">BUTTON (GPIO0)</span>
        <span class="gpio-state" id="btnState"></span>
        <span id="btnText">--</span>
      </div>

      <div class="gpio-control">
        <span class="gpio-name">RELAY (GPIO13)</span>
        <span class="gpio-state" id="relayState"></span>
        <div>
          <button class="btn btn-on btn-sm" onclick="setGpio('relay', true)">ON</button>
          <button class="btn btn-off btn-sm" onclick="setGpio('relay', false)">OFF</button>
        </div>
      </div>

      <div class="gpio-control">
        <span class="gpio-name">LED WiFi (GPIO5)</span>
        <span class="gpio-state" id="ledWifiState"></span>
        <div>
          <button class="btn btn-on btn-sm" onclick="setGpio('ledWifi', true)">ON</button>
          <button class="btn btn-off btn-sm" onclick="setGpio('ledWifi', false)">OFF</button>
        </div>
      </div>

      <div class="gpio-control">
        <span class="gpio-name">LED Relay (GPIO18)</span>
        <span class="gpio-state" id="ledRelayState"></span>
        <div>
          <button class="btn btn-on btn-sm" onclick="setGpio('ledRelay', true)">ON</button>
          <button class="btn btn-off btn-sm" onclick="setGpio('ledRelay', false)">OFF</button>
        </div>
      </div>
    </div>

    <!-- Display TM1621 - EXPANDED -->
    <div class="card">
      <h2>Display TM1621</h2>

      <!-- TEST MODE TOGGLE - IMPORTANTE -->
      <div id="testModeBox" style="background:#300;border:2px solid #f00;padding:10px;margin-bottom:15px;border-radius:5px;">
        <div style="display:flex;justify-content:space-between;align-items:center;">
          <span style="color:#ff0;font-weight:bold;">⚠ MODALITA' TEST</span>
          <span id="testModeStatus" style="font-weight:bold;">OFF</span>
        </div>
        <p style="color:#aaa;font-size:11px;margin:5px 0;">Blocca aggiornamenti automatici. Si disattiva uscendo dalla pagina debug.</p>
        <div style="margin-top:8px;">
          <button class="btn btn-on" onclick="setTestMode(true)">TEST ON</button>
          <button class="btn btn-off" onclick="setTestMode(false)">TEST OFF</button>
        </div>
      </div>

      <!-- Display Status -->
      <div class="status-row">
        <span class="status-label">Initialized</span>
        <span class="status-value" id="dispInit">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">LCD Power</span>
        <span class="status-value" id="dispOn">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Content</span>
        <span class="status-value" id="dispContent">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Updates</span>
        <span class="status-value" id="dispUpdates">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Buffer</span>
        <span class="status-value" id="dispBuffer" style="font-size:11px;">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Params</span>
        <span class="status-value" id="dispParams" style="font-size:10px;">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Cmd Mode</span>
        <span class="status-value" id="dispCmdMode">--</span>
      </div>

      <!-- Display Power Control -->
      <h3>Power Control</h3>
      <div style="margin-bottom:10px;">
        <button class="btn btn-on" onclick="testDisplay('on')">LCD ON</button>
        <button class="btn btn-off" onclick="testDisplay('off')">LCD OFF</button>
        <button class="btn btn-diag" onclick="testDisplay('reinit')">Re-Init</button>
      </div>

      <!-- Standard Display Tests -->
      <h3>Content Tests</h3>
      <div style="display:flex;flex-wrap:wrap;gap:5px;margin-bottom:10px;">
        <button class="btn btn-test btn-sm" onclick="testDisplay('clear')">Clear</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('all_on')">All ON</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('all_off')">All OFF</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('time')">12:34</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('bell')">bELL</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('loading')">----</button>
      </div>

      <!-- LOW LEVEL DIAGNOSTICS -->
      <h3>Memory Fill</h3>
      <div style="display:flex;gap:5px;flex-wrap:wrap;margin-bottom:10px;">
        <select id="fillAddr" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="0">Addr 0x00</option>
          <option value="16" selected>Addr 0x10</option>
        </select>
        <select id="fillValue" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="255">0xFF</option>
          <option value="0">0x00</option>
          <option value="170">0xAA</option>
          <option value="85">0x55</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="testFill()">Fill</button>
        <button class="btn btn-diag btn-sm" onclick="testDisplay('test_pins')">Test Pins</button>
      </div>

      <!-- Raw Write 4-bit (come da datasheet) -->
      <h3>Raw Write 4-bit (datasheet)</h3>
      <p style="color:#888;font-size:10px;margin:0 0 5px 0;">TM1621 RAM = 32x4 bit. Ogni indirizzo = 4 bit.</p>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <span style="color:#888;">Addr</span>
        <input type="number" id="raw4Addr" value="0" min="0" max="31" style="width:50px;padding:6px;background:#000;color:#0f0;border:1px solid #333;">
        <span style="color:#888;">Data</span>
        <select id="raw4Data" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="15">0xF (1111)</option>
          <option value="0">0x0 (0000)</option>
          <option value="1">0x1 (0001)</option>
          <option value="2">0x2 (0010)</option>
          <option value="4">0x4 (0100)</option>
          <option value="8">0x8 (1000)</option>
          <option value="5">0x5 (0101)</option>
          <option value="10">0xA (1010)</option>
        </select>
        <button class="btn btn-warn btn-sm" onclick="testRaw4bit()">Write 4bit</button>
      </div>

      <!-- Raw Write 8-bit (legacy) -->
      <h3>Raw Write 8-bit (2 nibble)</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <span style="color:#888;">Addr 0x</span>
        <input type="text" id="rawAddr" value="10" style="width:40px;padding:6px;background:#000;color:#0f0;border:1px solid #333;">
        <span style="color:#888;">Data 0x</span>
        <input type="text" id="rawData" value="FF" style="width:40px;padding:6px;background:#000;color:#0f0;border:1px solid #333;">
        <button class="btn btn-diag btn-sm" onclick="testRawWrite()">Write 8bit</button>
      </div>

      <!-- Tasmota Style Write (byte consecutivi) -->
      <h3>Multi-byte Write @ 0x10</h3>
      <div style="display:flex;gap:5px;flex-wrap:wrap;margin-bottom:10px;">
        <button class="btn btn-diag btn-sm" onclick="testDisplay('multi_1byte')">1x 0xFF</button>
        <button class="btn btn-diag btn-sm" onclick="testDisplay('multi_2byte')">2x 0xFF</button>
        <button class="btn btn-warn btn-sm" onclick="testDisplay('tasmota_8xff')">8x 0xFF</button>
        <button class="btn btn-warn btn-sm" onclick="testDisplay('tasmota_8x00')">8x 0x00</button>
        <button class="btn btn-warn btn-sm" onclick="testDisplay('tasmota_8888')">8888</button>
      </div>

      <!-- Send Command -->
      <h3>Send Command</h3>
      <div style="display:flex;gap:5px;align-items:center;">
        <select id="cmdSelect" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="1">0x01 SYS_EN</option>
          <option value="0">0x00 SYS_DIS</option>
          <option value="3">0x03 LCD_ON</option>
          <option value="2">0x02 LCD_OFF</option>
          <option value="41">0x29 BIAS 1/3 4COM</option>
          <option value="40">0x28 BIAS 1/2 4COM</option>
          <option value="37">0x25 BIAS 1/3 3COM</option>
          <option value="4">0x04 TIMER_DIS</option>
          <option value="5">0x05 WDT_DIS</option>
          <option value="8">0x08 TONE_OFF</option>
          <option value="128">0x80 IRQ_DIS</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="testCmd()">Send</button>
      </div>
    </div>

    <!-- TM1621 Configuration -->
    <div class="card">
      <h2>TM1621 Config</h2>
      <p style="color:#888;font-size:11px;margin-bottom:10px;">Modifica parametri e premi Re-Init per applicare</p>

      <!-- Modalita' Comando -->
      <h3>Modalita' Comando</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:15px;">
        <select id="cmdMode" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="0" selected>emsyscode (LSB, 2us) - DEFAULT</option>
          <option value="1">Tasmota (MSB, 10us)</option>
        </select>
        <button class="btn btn-warn btn-sm" onclick="setCmdMode()">Applica</button>
        <span id="cmdModeStatus" style="color:#888;font-size:11px;margin-left:5px;"></span>
      </div>

      <!-- Presets -->
      <h3>Preset Configurazioni</h3>
      <div style="display:flex;flex-wrap:wrap;gap:5px;margin-bottom:15px;">
        <button class="btn btn-warn btn-sm" onclick="testDisplay('preset_tasmota')">Tasmota</button>
        <button class="btn btn-warn btn-sm" onclick="testDisplay('preset_esphome')">ESPHome</button>
        <button class="btn btn-warn btn-sm" onclick="testDisplay('preset_fast')">Fast</button>
        <button class="btn btn-warn btn-sm" onclick="testDisplay('preset_slow')">Slow</button>
      </div>

      <!-- Timing -->
      <h3>Pulse Width (us)</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <select id="pulseWidth" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="2">2 us (fast)</option>
          <option value="5">5 us</option>
          <option value="10" selected>10 us (default)</option>
          <option value="20">20 us (slow)</option>
          <option value="50">50 us (very slow)</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="setParam('set_pulse', 'pulseWidth')">Set</button>
      </div>

      <!-- Memory Address -->
      <h3>Memory Start Address</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <select id="memAddr" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="0">0x00 (some devices)</option>
          <option value="16" selected>0x10 (Tasmota/Sonoff)</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="setParam('set_addr', 'memAddr')">Set</button>
      </div>

      <!-- Bit Order -->
      <h3>Data Bit Order</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <select id="bitOrder" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="1" selected>LSB first (standard)</option>
          <option value="0">MSB first</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="setParam('set_lsb', 'bitOrder')">Set</button>
      </div>

      <!-- BIAS -->
      <h3>LCD BIAS Configuration</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <select id="biasCmd" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="41" selected>0x29 - 1/3 bias, 4 COM (POWR316D)</option>
          <option value="40">0x28 - 1/2 bias, 4 COM</option>
          <option value="37">0x25 - 1/3 bias, 3 COM</option>
          <option value="36">0x24 - 1/2 bias, 3 COM</option>
          <option value="33">0x21 - 1/3 bias, 2 COM</option>
          <option value="32">0x20 - 1/2 bias, 2 COM</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="setParam('set_bias', 'biasCmd')">Set</button>
      </div>

      <!-- Reset Mode -->
      <h3>Reset Sequence</h3>
      <div style="display:flex;gap:5px;align-items:center;margin-bottom:10px;">
        <select id="resetMode" style="padding:5px;background:#000;color:#0f0;border:1px solid #333;">
          <option value="0" selected>Tasmota style</option>
          <option value="1">ESPHome style</option>
          <option value="2">Minimal</option>
          <option value="3">Extended</option>
        </select>
        <button class="btn btn-diag btn-sm" onclick="setParam('set_reset', 'resetMode')">Set</button>
      </div>

      <div style="margin-top:15px;">
        <button class="btn btn-on" onclick="testDisplay('reinit')">Apply & Re-Init Display</button>
        <button class="btn btn-test" onclick="refreshParams()">Refresh Params</button>
      </div>
    </div>

    <!-- Display Pin Info -->
    <div class="card">
      <h2>Hardware Info</h2>
      <div class="pin-row">
        <span class="pin-label">CS</span>
        <span class="pin-gpio">GPIO25</span>
      </div>
      <div class="pin-row">
        <span class="pin-label">DATA</span>
        <span class="pin-gpio">GPIO14</span>
      </div>
      <div class="pin-row">
        <span class="pin-label">RD</span>
        <span class="pin-gpio">GPIO26</span>
      </div>
      <div class="pin-row">
        <span class="pin-label">WR</span>
        <span class="pin-gpio">GPIO27</span>
      </div>
      <p style="color:#888;font-size:11px;margin-top:10px;">
        Chip: TM1621 (Titan Micro)<br>
        LCD: 2x4 digits, 7-segment<br>
        Protocol: 4-wire serial (bit-bang)<br>
        Sources: <a href="https://github.com/arendst/Tasmota/blob/development/tasmota/tasmota_xdrv_driver/xdrv_87_esp32_sonoff_tm1621.ino" style="color:#0ff;">Tasmota</a>,
        <a href="https://esphome.io/components/display/tm1621/" style="color:#0ff;">ESPHome</a>,
        <a href="https://espeasy.readthedocs.io/en/latest/Plugin/P148.html" style="color:#0ff;">ESPEasy</a>
      </p>
    </div>

    <!-- System Info -->
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
        <span class="status-label">CPU Freq</span>
        <span class="status-value" id="cpuFreq">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Free Heap</span>
        <span class="status-value" id="freeHeap">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Min Free Heap</span>
        <span class="status-value" id="minFreeHeap">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Flash Size</span>
        <span class="status-value" id="flashSize">--</span>
      </div>
    </div>

    <!-- WiFi Info -->
    <div class="card">
      <h2>WiFi Status</h2>
      <div class="status-row">
        <span class="status-label">State</span>
        <span class="status-value" id="wifiState">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">SSID</span>
        <span class="status-value" id="wifiSSID">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">IP</span>
        <span class="status-value" id="wifiIP">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">MAC</span>
        <span class="status-value" id="wifiMAC">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">RSSI</span>
        <span class="status-value" id="wifiRSSI">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Channel</span>
        <span class="status-value" id="wifiChannel">--</span>
      </div>
    </div>

    <!-- NTP Info -->
    <div class="card">
      <h2>NTP / Time</h2>
      <div class="status-row">
        <span class="status-label">NTP Synced</span>
        <span class="status-value" id="ntpSynced">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Time Set</span>
        <span class="status-value" id="timeSet">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Current Time</span>
        <span class="status-value" id="currentTime">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Current Date</span>
        <span class="status-value" id="currentDate">--</span>
      </div>
    </div>

    <!-- Bells Info -->
    <div class="card">
      <h2>Bells Status</h2>
      <div class="status-row">
        <span class="status-label">Bell Count</span>
        <span class="status-value" id="bellCount">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Global Enabled</span>
        <span class="status-value" id="globalEnabled">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Is Ringing</span>
        <span class="status-value" id="isRinging">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Ringing Bell ID</span>
        <span class="status-value" id="ringingBellId">--</span>
      </div>
    </div>
  </div>

  <!-- Actions -->
  <div class="card" style="margin-top:20px;">
    <h2>Actions</h2>
    <button class="btn btn-warn" onclick="restart()">RESTART DEVICE</button>
    <a href="/" class="btn btn-test" style="text-decoration:none;display:inline-block;">Back to Main</a>
  </div>

  <!-- Log -->
  <div class="card" style="margin-top:20px;">
    <h2>Activity Log</h2>
    <div class="log" id="log"></div>
  </div>

  <script>
    let eventSource = null;
    let sseConnected = false;

    function log(msg, type = 'info') {
      const logDiv = document.getElementById('log');
      const time = new Date().toLocaleTimeString();
      const cls = type === 'error' ? 'log-error' : (type === 'warn' ? 'log-warn' : '');
      logDiv.innerHTML = `<div class="log-entry ${cls}"><span class="log-time">[${time}]</span> ${msg}</div>` + logDiv.innerHTML;
      if (logDiv.children.length > 100) {
        logDiv.removeChild(logDiv.lastChild);
      }
    }

    async function api(url, options = {}) {
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 10000);

      try {
        const res = await fetch(url, {
          ...options,
          headers: { 'Content-Type': 'application/json' },
          signal: controller.signal
        });
        clearTimeout(timeout);
        return await res.json();
      } catch (e) {
        clearTimeout(timeout);
        if (e.name === 'AbortError') {
          log('Timeout richiesta', 'warn');
        } else {
          log('ERROR: ' + e.message, 'error');
        }
        return null;
      }
    }

    // === SSE per dati real-time (GPIO, tempo, stato) ===
    function connectSSE() {
      if (eventSource) eventSource.close();

      log('SSE: Connecting...');
      eventSource = new EventSource('/api/events');

      eventSource.onopen = () => {
        log('SSE: Connected (real-time updates active)');
        sseConnected = true;
        document.getElementById('lastUpdate').textContent = 'SSE: Connected';
      };

      eventSource.onmessage = (event) => {
        try {
          const d = JSON.parse(event.data);
          updateFromSSE(d);
        } catch (e) {
          log('SSE parse error', 'error');
        }
      };

      eventSource.onerror = () => {
        log('SSE: Disconnected, reconnecting...', 'warn');
        sseConnected = false;
        document.getElementById('lastUpdate').textContent = 'SSE: Reconnecting...';
        eventSource.close();
        setTimeout(connectSSE, 3000);
      };
    }

    function updateFromSSE(d) {
      // GPIO states
      if (d.btn !== undefined) {
        setStateIndicator('btnState', d.btn === 1);
        document.getElementById('btnText').textContent = d.btn === 1 ? 'PRESSED' : 'Released';
      }
      if (d.relay !== undefined) setStateIndicator('relayState', d.relay === 1);
      if (d.ledW !== undefined) setStateIndicator('ledWifiState', d.ledW === 1);
      if (d.ledR !== undefined) setStateIndicator('ledRelayState', d.ledR === 1);

      // Time
      if (d.h !== undefined && d.m !== undefined) {
        const timeStr = String(d.h).padStart(2,'0') + ':' + String(d.m).padStart(2,'0');
        document.getElementById('currentTime').textContent = timeStr;
      }

      // Ringing status
      if (d.ring !== undefined) setBoolValue('isRinging', d.ring === 1);
      if (d.ringId !== undefined) document.getElementById('ringingBellId').textContent = d.ringId || '-';

      // Uptime
      if (d.up !== undefined) document.getElementById('uptime').textContent = formatUptime(d.up);

      // Heap
      if (d.heap !== undefined) document.getElementById('freeHeap').textContent = formatBytes(d.heap);

      // WiFi state
      if (d.wifi !== undefined) {
        const names = ['Disconnesso', 'Connessione...', 'Connesso', 'Sincronizzato', 'AP Mode'];
        document.getElementById('wifiState').textContent = names[d.wifi] || 'Sconosciuto';
      }

      // NTP
      if (d.ntp !== undefined) setBoolValue('ntpSynced', d.ntp === 1);

      // Global
      if (d.global !== undefined) setBoolValue('globalEnabled', d.global === 1);

      // Update timestamp
      document.getElementById('lastUpdate').textContent = 'SSE: ' + new Date().toLocaleTimeString();
    }

    // === Polling per dati pesanti (display buffer, WiFi details, heap min) ===
    async function loadHeavyData() {
      const data = await api('/api/debug/status');
      if (!data) return;

      // Display Status (dati pesanti)
      setBoolValue('dispInit', data.dispInit);
      setBoolValue('dispOn', data.dispOn);
      document.getElementById('dispContent').textContent = data.dispContent || '-';
      document.getElementById('dispUpdates').textContent = data.dispUpdates || '0';
      document.getElementById('dispBuffer').textContent = data.dispBuffer || '-';
      document.getElementById('dispParams').textContent = data.dispParams || '-';
      document.getElementById('dispCmdMode').textContent = data.cmdModeName || '--';
      if (data.cmdMode !== undefined) {
        document.getElementById('cmdMode').value = data.cmdMode;
      }
      // Test Mode status
      if (data.testMode !== undefined) {
        updateTestModeUI(data.testMode);
      }

      // System Info (dati che cambiano poco)
      document.getElementById('version').textContent = data.version;
      document.getElementById('cpuFreq').textContent = data.cpuFreq + ' MHz';
      document.getElementById('minFreeHeap').textContent = formatBytes(data.minFreeHeap);
      document.getElementById('flashSize').textContent = formatBytes(data.flashSize);

      // WiFi Info dettagliato
      document.getElementById('wifiState').textContent = data.wifiStateName;
      document.getElementById('wifiSSID').textContent = data.wifiSSID || '-';
      document.getElementById('wifiIP').textContent = data.wifiIP;
      document.getElementById('wifiMAC').textContent = data.wifiMAC;
      document.getElementById('wifiRSSI').textContent = data.wifiRSSI + ' dBm';
      document.getElementById('wifiChannel').textContent = data.wifiChannel;

      // NTP Info
      setBoolValue('ntpSynced', data.ntpSynced);
      setBoolValue('timeSet', data.timeSet);
      document.getElementById('currentDate').textContent = data.currentDate;

      // Bells Info
      document.getElementById('bellCount').textContent = data.bellCount;
      setBoolValue('globalEnabled', data.globalEnabled);
      setBoolValue('isRinging', data.isRinging);
      document.getElementById('ringingBellId').textContent = data.ringingBellId || '-';
    }

    function setStateIndicator(id, state) {
      const el = document.getElementById(id);
      el.className = 'gpio-state ' + (state ? 'on' : 'off');
    }

    function setBoolValue(id, value) {
      const el = document.getElementById(id);
      el.textContent = value ? 'YES' : 'NO';
      el.className = 'status-value ' + (value ? '' : 'off');
    }

    function formatBytes(bytes) {
      if (bytes < 1024) return bytes + ' B';
      if (bytes < 1024*1024) return (bytes/1024).toFixed(1) + ' KB';
      return (bytes/1024/1024).toFixed(1) + ' MB';
    }

    function formatUptime(seconds) {
      const h = Math.floor(seconds / 3600);
      const m = Math.floor((seconds % 3600) / 60);
      const s = seconds % 60;
      return `${h}h ${m}m ${s}s`;
    }

    async function setGpio(gpio, state) {
      log(`Setting ${gpio} -> ${state ? 'ON' : 'OFF'}`);
      const result = await api('/api/debug/gpio', {
        method: 'POST',
        body: JSON.stringify({ gpio, state })
      });
      if (result && result.success) {
        log(`${gpio} set successfully`);
      }
      // SSE aggiornera' automaticamente lo stato GPIO
    }

    async function testDisplay(test) {
      log(`Display test: ${test}`);
      const result = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test })
      });
      if (result) {
        if (result.success) {
          log(`Display: ${result.message || 'OK'}`);
        } else {
          log(`Display error: ${result.error || 'unknown'}`, 'error');
        }
      }
      // Ricarica dati display dopo test
      setTimeout(loadHeavyDataSafe, 500);
    }

    async function testDisplayNumber() {
      const value = parseInt(document.getElementById('displayNum').value) || 0;
      log(`Display number: ${value}`);
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'number', value })
      });
      setTimeout(loadHeavyDataSafe, 500);
    }

    async function testRaw4bit() {
      const addr = parseInt(document.getElementById('raw4Addr').value) || 0;
      const data = parseInt(document.getElementById('raw4Data').value) || 0x0F;
      log(`Raw 4bit: addr=${addr} data=0x${data.toString(16).toUpperCase()}`, 'warn');
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'raw4', addr, data })
      });
      setTimeout(loadHeavyDataSafe, 500);
    }

    async function testRawWrite() {
      const addr = parseInt(document.getElementById('rawAddr').value, 16) || 0x10;
      const data = parseInt(document.getElementById('rawData').value, 16) || 0xFF;
      log(`Raw 8bit: addr=0x${addr.toString(16).toUpperCase()} data=0x${data.toString(16).toUpperCase()}`, 'warn');
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'raw', addr, data })
      });
      setTimeout(loadHeavyDataSafe, 500);
    }

    async function testFill() {
      const addr = parseInt(document.getElementById('fillAddr').value) || 0;
      const value = parseInt(document.getElementById('fillValue').value) || 255;
      log(`Fill memory: addr=0x${addr.toString(16).toUpperCase()} value=0x${value.toString(16).toUpperCase()}`);
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'fill', addr, value, count: 16 })
      });
      setTimeout(loadHeavyDataSafe, 500);
    }

    async function testCmd() {
      const cmd = parseInt(document.getElementById('cmdSelect').value) || 3;
      log(`Send command: 0x${cmd.toString(16).toUpperCase()}`, 'warn');
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'cmd', cmd })
      });
    }

    async function setParam(test, selectId) {
      const value = parseInt(document.getElementById(selectId).value);
      log(`Set param ${test}: ${value}`);
      const result = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test, value })
      });
      if (result && result.message) {
        log(`TM1621: ${result.message}`);
      }
    }

    async function setTestMode(enable) {
      log(`Setting test mode: ${enable ? 'ON' : 'OFF'}`, 'warn');
      const result = await api(`/api/display/test_mode?enable=${enable ? 1 : 0}`);
      if (result && result.success) {
        log(`Test mode: ${result.message}`);
        updateTestModeUI(enable);
      } else {
        log('Errore cambio test mode', 'error');
      }
    }

    function updateTestModeUI(enabled) {
      const box = document.getElementById('testModeBox');
      const status = document.getElementById('testModeStatus');
      if (enabled) {
        box.style.background = '#030';
        box.style.borderColor = '#0f0';
        status.textContent = 'ON - ATTIVO';
        status.style.color = '#0f0';
      } else {
        box.style.background = '#300';
        box.style.borderColor = '#f00';
        status.textContent = 'OFF';
        status.style.color = '#888';
      }
    }

    async function setCmdMode() {
      const mode = parseInt(document.getElementById('cmdMode').value);
      const name = mode === 0 ? 'emsyscode' : 'Tasmota';
      log(`Setting cmd mode: ${name}`, 'warn');
      const result = await api(`/api/display/set_cmd_mode?mode=${mode}`);
      if (result && result.success) {
        log(`Cmd mode: ${result.message}`);
        document.getElementById('cmdModeStatus').textContent = name;
        document.getElementById('dispCmdMode').textContent = name;
      } else {
        log('Errore cambio cmd mode', 'error');
      }
      setTimeout(loadHeavyDataSafe, 500);
    }

    async function refreshParams() {
      log('Refreshing TM1621 params...');
      const result = await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'get_params' })
      });
      if (result) {
        document.getElementById('dispParams').textContent = result.params || '-';
        document.getElementById('dispBuffer').textContent = result.buffer || '-';
        document.getElementById('dispContent').textContent = result.content || '-';
        document.getElementById('dispUpdates').textContent = result.updates || '0';
        log(`Params: ${result.params}`);
      }
    }

    async function restart() {
      if (!confirm('Riavviare il dispositivo?')) return;
      log('Restarting device...', 'warn');
      await api('/api/debug/restart', { method: 'POST' });
    }

    // Request management for heavy data
    let heavyDataPending = false;

    async function loadHeavyDataSafe() {
      if (heavyDataPending) return;
      heavyDataPending = true;
      try {
        await loadHeavyData();
      } finally {
        heavyDataPending = false;
      }
    }

    // Disattiva test mode quando si lascia la pagina
    window.addEventListener('beforeunload', function() {
      // Chiamata sincrona per disattivare test mode
      navigator.sendBeacon('/api/display/test_mode?enable=0');
    });

    // Init
    log('Debug console v2.2 initialized');
    log('SSE for real-time GPIO/time updates');
    log('Polling every 30s for heavy data (display, WiFi)');
    log('Test mode si disattiva automaticamente uscendo dalla pagina');

    // Carica dati pesanti una volta all'avvio
    loadHeavyDataSafe();

    // Connetti SSE per aggiornamenti real-time
    connectSSE();

    // Polling dati pesanti ogni 30 secondi (display buffer, WiFi details)
    setInterval(loadHeavyDataSafe, 30000);
  </script>
</body>
</html>
)rawliteral";

#endif // DEBUG_PAGE_H

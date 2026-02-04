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

      <!-- Display Power Control -->
      <h3>Power Control</h3>
      <div style="margin-bottom:10px;">
        <button class="btn btn-on" onclick="testDisplay('on')">LCD ON</button>
        <button class="btn btn-off" onclick="testDisplay('off')">LCD OFF</button>
      </div>

      <!-- Standard Display Tests -->
      <h3>Content Tests</h3>
      <div style="display:flex;flex-wrap:wrap;gap:5px;margin-bottom:10px;">
        <button class="btn btn-test btn-sm" onclick="testDisplay('clear')">Clear</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('all')">All ON</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('time')">12:34</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('bell')">bELL</button>
        <button class="btn btn-test btn-sm" onclick="testDisplay('loading')">----</button>
      </div>
      <div style="margin-bottom:15px;">
        <input type="number" id="displayNum" placeholder="Numero" style="width:80px;padding:6px;background:#000;color:#0f0;border:1px solid #333;">
        <button class="btn btn-test btn-sm" onclick="testDisplayNumber()">Show</button>
      </div>

      <!-- LOW LEVEL DIAGNOSTICS -->
      <h3>Low-Level Diagnostics</h3>
      <div style="display:flex;flex-wrap:wrap;gap:5px;margin-bottom:10px;">
        <button class="btn btn-diag btn-sm" onclick="testDisplay('fill_ff')">Fill 0xFF</button>
        <button class="btn btn-diag btn-sm" onclick="testDisplay('fill_00')">Fill 0x00</button>
        <button class="btn btn-diag btn-sm" onclick="testDisplay('fill_aa')">Fill 0xAA</button>
        <button class="btn btn-diag btn-sm" onclick="testDisplay('fill_55')">Fill 0x55</button>
      </div>
      <div style="display:flex;flex-wrap:wrap;gap:5px;margin-bottom:10px;">
        <button class="btn btn-diag btn-sm" onclick="testDisplay('test_pins')">Test Pins</button>
        <button class="btn btn-diag btn-sm" onclick="testDisplay('reinit')">Re-Init</button>
      </div>

      <!-- Raw Write -->
      <h3>Raw Write (Addr + Data)</h3>
      <div style="display:flex;gap:5px;align-items:center;">
        <span style="color:#888;">0x</span>
        <input type="text" id="rawAddr" placeholder="10" style="width:40px;padding:6px;background:#000;color:#0f0;border:1px solid #333;">
        <span style="color:#888;">0x</span>
        <input type="text" id="rawData" placeholder="FF" style="width:40px;padding:6px;background:#000;color:#0f0;border:1px solid #333;">
        <button class="btn btn-diag btn-sm" onclick="testRawWrite()">Write</button>
      </div>
    </div>

    <!-- Display Pin Info -->
    <div class="card">
      <h2>Display Pins (TM1621)</h2>
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
        Source: ESPHome, Tasmota<br>
        Protocol: 4-wire serial (bit-bang)
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

    async function testRawWrite() {
      const addr = parseInt(document.getElementById('rawAddr').value, 16) || 0x10;
      const data = parseInt(document.getElementById('rawData').value, 16) || 0xFF;
      log(`Raw write: addr=0x${addr.toString(16).toUpperCase()} data=0x${data.toString(16).toUpperCase()}`, 'warn');
      await api('/api/debug/display', {
        method: 'POST',
        body: JSON.stringify({ test: 'raw', addr, data })
      });
      setTimeout(loadHeavyDataSafe, 500);
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

    // Init
    log('Debug console v2.1 initialized');
    log('SSE for real-time GPIO/time updates');
    log('Polling every 30s for heavy data (display, WiFi)');

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

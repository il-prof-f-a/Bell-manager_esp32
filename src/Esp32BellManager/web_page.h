#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <pgmspace.h>

// ============================================
// Pagina Web Bell-Manager v2.0
// Con configurazione WiFi e NTP
// ============================================

const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Bell-Manager</title>
  <style>
    * { box-sizing: border-box; }
    body { margin: 0; font-family: Arial, sans-serif; background: #f0f2f5; }

    .header {
      display: flex; justify-content: space-between; align-items: center;
      background: #0180ff; color: white; padding: 15px 20px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.2);
    }
    .header-section { text-align: center; }
    .header-title { font-size: 22px; font-weight: bold; }
    .header-label { font-size: 14px; opacity: 0.8; }
    .header-value { font-size: 18px; font-weight: 600; }

    .status-dot {
      display: inline-block; width: 10px; height: 10px;
      border-radius: 50%; margin-right: 6px;
    }
    .status-connected { background: #4caf50; }
    .status-synced { background: #4caf50; box-shadow: 0 0 6px #4caf50; }
    .status-connecting { background: #ff9800; }
    .status-disconnected { background: #f44336; }
    .status-ap { background: #9c27b0; }

    .container {
      max-width: 1200px; margin: 20px auto; padding: 20px;
      background: white; border-radius: 15px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }

    .btn-bar {
      display: flex; flex-wrap: wrap; gap: 10px;
      justify-content: center; margin-bottom: 20px;
    }
    .btn {
      padding: 12px 20px; font-size: 14px; font-weight: 600;
      border: none; border-radius: 25px; cursor: pointer;
      background: #494949; color: white; transition: 0.3s;
    }
    .btn:hover { background: #000; }
    .btn-primary { background: #0180ff; }
    .btn-primary:hover { background: #0066cc; }
    .btn-danger { background: #f44336; }
    .btn-danger:hover { background: #d32f2f; }

    .modal {
      display: none; position: fixed; top: 0; left: 0;
      width: 100%; height: 100%; background: rgba(0,0,0,0.5);
      z-index: 1000; justify-content: center; align-items: center;
    }
    .modal.active { display: flex; }
    .modal-content {
      background: white; padding: 25px; border-radius: 15px;
      max-width: 500px; width: 90%; max-height: 90vh; overflow-y: auto;
    }
    .modal-title { margin: 0 0 20px; text-align: center; }

    .form-group { margin-bottom: 15px; }
    .form-group label { display: block; margin-bottom: 5px; font-weight: 600; }
    .form-group input, .form-group select {
      width: 100%; padding: 10px; border: 1px solid #ddd;
      border-radius: 8px; font-size: 14px;
    }

    .form-row { display: flex; gap: 15px; }
    .form-row .form-group { flex: 1; }

    .switch-container { display: flex; align-items: center; gap: 10px; }
    .switch {
      position: relative; width: 50px; height: 26px;
    }
    .switch input { opacity: 0; width: 0; height: 0; }
    .switch .slider {
      position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
      background: #ccc; border-radius: 26px; transition: 0.3s;
    }
    .switch .slider:before {
      content: ""; position: absolute; height: 20px; width: 20px;
      left: 3px; bottom: 3px; background: white;
      border-radius: 50%; transition: 0.3s;
    }
    .switch input:checked + .slider { background: #0180ff; }
    .switch input:checked + .slider:before { transform: translateX(24px); }

    .days-grid {
      display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px;
    }
    .day-btn {
      padding: 8px; border: 2px solid #ddd; border-radius: 8px;
      background: white; cursor: pointer; font-weight: 600; transition: 0.2s;
    }
    .day-btn.active { border-color: #0180ff; background: #e3f2fd; }

    .table-container { overflow-x: auto; }
    table { width: 100%; border-collapse: collapse; }
    th, td { padding: 12px; text-align: center; border-bottom: 1px solid #eee; }
    th { background: #f5f5f5; font-size: 13px; text-transform: uppercase; }
    tr:hover { background: #f9f9f9; }

    .actions { display: flex; gap: 8px; justify-content: center; }
    .action-btn {
      border: none; background: none; cursor: pointer;
      font-size: 18px; padding: 5px; transition: 0.2s;
    }
    .action-btn:hover { transform: scale(1.2); }
    .action-btn.edit { color: #0180ff; }
    .action-btn.delete { color: #f44336; }

    .info-box {
      background: #e3f2fd; padding: 15px; border-radius: 10px;
      margin-bottom: 15px;
    }
    .info-row { display: flex; justify-content: space-between; margin: 5px 0; }

    .wifi-status {
      padding: 10px 15px; border-radius: 8px; margin-bottom: 15px;
      display: flex; align-items: center; gap: 10px;
    }
    .wifi-status.connected { background: #e8f5e9; color: #2e7d32; }
    .wifi-status.ap-mode { background: #f3e5f5; color: #7b1fa2; }
    .wifi-status.disconnected { background: #ffebee; color: #c62828; }

    .log-list {
      max-height: 200px; overflow-y: auto; background: #f5f5f5;
      padding: 10px; border-radius: 8px; font-size: 13px;
    }
    .log-item { padding: 5px 0; border-bottom: 1px solid #ddd; }

    .footer { text-align: center; padding: 20px; color: #666; font-size: 13px; }
  </style>
</head>
<body>
  <div class="header">
    <div class="header-section">
      <div class="header-label">Ora</div>
      <div class="header-value" id="currentTime">--:--</div>
      <div class="header-label" id="currentDate">--/--/----</div>
    </div>
    <div class="header-section">
      <div class="header-title">Bell-Manager</div>
      <div id="wifiStatusHeader">
        <span class="status-dot status-disconnected"></span>
        <span id="wifiStatusText">Disconnesso</span>
      </div>
    </div>
    <div class="header-section">
      <div class="header-label">Prossima</div>
      <div class="header-value" id="nextBellType">Nessuna</div>
      <div class="header-label" id="nextBellTime">--:--</div>
    </div>
  </div>

  <div class="container">
    <h2 id="institutionName" style="text-align:center;margin-bottom:20px;">Istituzione</h2>

    <div class="btn-bar">
      <button class="btn btn-primary" onclick="openModal('addBellModal')">+ Aggiungi Campanella</button>
      <button class="btn" onclick="openModal('settingsModal')">Impostazioni</button>
      <button class="btn" onclick="openModal('wifiModal')">WiFi</button>
    </div>

    <div class="table-container">
      <table>
        <thead>
          <tr>
            <th>Ora</th>
            <th>Giorni</th>
            <th>Tipo</th>
            <th>Durata</th>
            <th>Attiva</th>
            <th>Azioni</th>
          </tr>
        </thead>
        <tbody id="bellsTable"></tbody>
      </table>
    </div>

    <div class="footer">Bell-Manager v2.0 - Made with love</div>
  </div>

  <!-- Modal Aggiungi/Modifica Campanella -->
  <div class="modal" id="addBellModal">
    <div class="modal-content">
      <h2 class="modal-title" id="bellModalTitle">Aggiungi Campanella</h2>
      <input type="hidden" id="editBellId">

      <div class="form-row">
        <div class="form-group">
          <label>Ora</label>
          <input type="time" id="bellTime">
        </div>
        <div class="form-group">
          <label>Durata (sec)</label>
          <input type="number" id="bellDuration" min="1" max="60" value="3">
        </div>
      </div>

      <div class="form-group">
        <label>Tipo</label>
        <input type="text" id="bellType" placeholder="Es: Intervallo, Fine lezione">
      </div>

      <div class="form-group">
        <label>Giorni</label>
        <div class="days-grid">
          <button type="button" class="day-btn active" data-day="0">Lun</button>
          <button type="button" class="day-btn active" data-day="1">Mar</button>
          <button type="button" class="day-btn active" data-day="2">Mer</button>
          <button type="button" class="day-btn active" data-day="3">Gio</button>
          <button type="button" class="day-btn active" data-day="4">Ven</button>
          <button type="button" class="day-btn" data-day="5">Sab</button>
          <button type="button" class="day-btn" data-day="6">Dom</button>
        </div>
      </div>

      <div class="btn-bar">
        <button class="btn btn-primary" onclick="saveBell()">Salva</button>
        <button class="btn btn-danger" onclick="closeModal('addBellModal')">Annulla</button>
      </div>
    </div>
  </div>

  <!-- Modal Impostazioni -->
  <div class="modal" id="settingsModal">
    <div class="modal-content">
      <h2 class="modal-title">Impostazioni</h2>

      <div class="form-group">
        <label>Nome Istituzione</label>
        <input type="text" id="settingsName" placeholder="Nome istituzione">
      </div>

      <div class="form-group">
        <label>Campanelle Globali</label>
        <div class="switch-container">
          <span>Disabilitate</span>
          <label class="switch">
            <input type="checkbox" id="settingsGlobal" checked>
            <span class="slider"></span>
          </label>
          <span>Abilitate</span>
        </div>
      </div>

      <div class="form-group">
        <label>Fuso Orario (GMT)</label>
        <select id="settingsTimezone">
          <option value="-43200">GMT-12</option>
          <option value="-39600">GMT-11</option>
          <option value="-36000">GMT-10</option>
          <option value="-32400">GMT-9</option>
          <option value="-28800">GMT-8</option>
          <option value="-25200">GMT-7</option>
          <option value="-21600">GMT-6</option>
          <option value="-18000">GMT-5</option>
          <option value="-14400">GMT-4</option>
          <option value="-10800">GMT-3</option>
          <option value="-7200">GMT-2</option>
          <option value="-3600">GMT-1</option>
          <option value="0">GMT+0</option>
          <option value="3600" selected>GMT+1 (Italia)</option>
          <option value="7200">GMT+2</option>
          <option value="10800">GMT+3</option>
          <option value="14400">GMT+4</option>
          <option value="18000">GMT+5</option>
          <option value="21600">GMT+6</option>
          <option value="25200">GMT+7</option>
          <option value="28800">GMT+8</option>
          <option value="32400">GMT+9</option>
          <option value="36000">GMT+10</option>
          <option value="39600">GMT+11</option>
          <option value="43200">GMT+12</option>
        </select>
      </div>

      <div class="form-group">
        <label>Ora Legale</label>
        <div class="switch-container">
          <span>No</span>
          <label class="switch">
            <input type="checkbox" id="settingsDST" checked>
            <span class="slider"></span>
          </label>
          <span>Si (+1 ora)</span>
        </div>
      </div>

      <div class="form-group">
        <label>Log Campanelle</label>
        <div class="log-list" id="bellLog">Nessun log</div>
      </div>

      <div class="btn-bar">
        <button class="btn btn-primary" onclick="saveSettings()">Salva</button>
        <button class="btn btn-danger" onclick="closeModal('settingsModal')">Chiudi</button>
      </div>
    </div>
  </div>

  <!-- Modal WiFi -->
  <div class="modal" id="wifiModal">
    <div class="modal-content">
      <h2 class="modal-title">Configurazione WiFi</h2>

      <div id="wifiStatusBox" class="wifi-status disconnected">
        <span class="status-dot"></span>
        <span id="wifiStatusDetail">Stato sconosciuto</span>
      </div>

      <div class="info-box">
        <div class="info-row"><span>IP:</span><span id="wifiIP">-</span></div>
        <div class="info-row"><span>SSID:</span><span id="wifiSSID">-</span></div>
        <div class="info-row"><span>NTP Sync:</span><span id="wifiNTP">-</span></div>
      </div>

      <div class="form-group">
        <label>Seleziona Rete WiFi</label>
        <div style="display:flex;gap:10px;">
          <select id="wifiNetworkList" style="flex:1;" onchange="selectNetwork(this.value)">
            <option value="">-- Seleziona rete --</option>
          </select>
          <button class="btn" onclick="scanNetworks()" id="scanBtn">Scansiona</button>
        </div>
        <div id="scanStatus" style="font-size:12px;color:#666;margin-top:5px;"></div>
      </div>

      <div class="form-group">
        <label>SSID (o inserisci manualmente)</label>
        <input type="text" id="newWifiSSID" placeholder="Nome rete WiFi">
      </div>

      <div class="form-group">
        <label>Password</label>
        <div style="display:flex;gap:10px;align-items:center;">
          <input type="password" id="newWifiPass" placeholder="Password WiFi" style="flex:1;">
          <button type="button" class="btn" onclick="togglePassword()" id="togglePassBtn" style="padding:10px 15px;">👁</button>
        </div>
      </div>

      <div class="btn-bar">
        <button class="btn btn-primary" onclick="saveWifi()">Salva e Riavvia</button>
        <button class="btn btn-danger" onclick="closeModal('wifiModal')">Chiudi</button>
      </div>
    </div>
  </div>

  <script>
    // === Stato locale ===
    let state = {
      bells: [],
      system: { name: '', global: true, version: '' },
      time: { h: 0, m: 0, ntp: false },
      wifi: { state: 0, name: '' },
      sched: { ring: false, ringId: 0, nextH: 0, nextM: 0, nextT: '' }
    };
    let editingId = null;
    let eventSource = null;

    // === API (semplice) ===
    async function api(url, opts = {}) {
      try {
        const ctrl = new AbortController();
        const t = setTimeout(() => ctrl.abort(), 30000); // 30 sec timeout
        const res = await fetch(url, {
          method: opts.method || 'GET',
          body: opts.body,
          signal: ctrl.signal,
          headers: { 'Content-Type': 'application/json' }
        });
        clearTimeout(t);
        return await res.json();
      } catch (e) {
        console.error('API:', url, e.message);
        return null;
      }
    }

    // === SSE - Aggiornamenti real-time ===
    function connectSSE() {
      if (eventSource) eventSource.close();

      console.log('SSE connecting...');
      eventSource = new EventSource('/api/events');

      eventSource.onopen = () => console.log('SSE connected');

      eventSource.onmessage = (ev) => {
        try {
          const d = JSON.parse(ev.data);
          handleSSEData(d);
        } catch (e) {}
      };

      eventSource.onerror = () => {
        console.log('SSE lost, reconnecting...');
        eventSource.close();
        setTimeout(connectSSE, 3000);
      };
    }

    // Gestisce dati SSE (stato completo ogni secondo)
    function handleSSEData(d) {
      // Tempo
      if (d.h !== undefined && d.m !== undefined) {
        state.time.h = d.h;
        state.time.m = d.m;
        updateTimeUI();
      }

      // WiFi
      if (d.wifi !== undefined) {
        state.wifi.state = d.wifi;
        updateWifiUI();
      }

      // NTP
      if (d.ntp !== undefined) {
        state.time.ntp = d.ntp === 1;
      }

      // Global
      if (d.global !== undefined) {
        state.system.global = d.global === 1;
      }

      // Ringing
      if (d.ring !== undefined) {
        state.sched.ring = d.ring === 1;
        state.sched.ringId = d.ringId || 0;
      }
    }

    function updateTimeUI() {
      const h = String(state.time.h).padStart(2, '0');
      const m = String(state.time.m).padStart(2, '0');
      document.getElementById('currentTime').textContent = h + ':' + m;
    }

    function updateNextBellUI() {
      const h = String(state.sched.nextH).padStart(2, '0');
      const m = String(state.sched.nextM).padStart(2, '0');
      document.getElementById('nextBellTime').textContent = h + ':' + m;
      document.getElementById('nextBellType').textContent = state.sched.nextT || 'Nessuna';
    }

    function updateWifiUI() {
      const hdr = document.getElementById('wifiStatusHeader');
      const s = state.wifi.state;
      let dot = 'status-disconnected', txt = 'Disconnesso';
      if (s === 4) { dot = 'status-ap'; txt = 'AP Mode'; }
      else if (s === 3) { dot = 'status-synced'; txt = 'Sincronizzato'; }
      else if (s === 2) { dot = 'status-connected'; txt = 'Connesso'; }
      else if (s === 1) { dot = 'status-connecting'; txt = 'Connessione...'; }
      hdr.innerHTML = '<span class="status-dot ' + dot + '"></span><span>' + txt + '</span>';
    }

    async function reloadBells() {
      const d = await api('/api/bells');
      if (d) {
        state.bells = d;
        renderBells();
      }
    }

    // === Caricamento iniziale (GET /api/state) ===
    async function loadState() {
      console.log('Loading state...');
      const d = await api('/api/state');

      if (!d) {
        setTimeout(loadState, 3000);
        return;
      }

      // System
      if (d.system) {
        state.system.name = d.system.name || '';
        state.system.global = d.system.global;
        state.system.version = d.system.version;
        document.getElementById('institutionName').textContent = state.system.name || 'Istituzione';
        document.getElementById('settingsName').value = state.system.name;
        document.getElementById('settingsGlobal').checked = state.system.global;
      }

      // Time
      if (d.time) {
        state.time.h = d.time.h;
        state.time.m = d.time.m;
        state.time.ntp = d.time.ntp;
        updateTimeUI();
        document.getElementById('currentDate').textContent = d.time.date || '';
        document.getElementById('settingsTimezone').value = d.time.gmt || 3600;
        document.getElementById('settingsDST').checked = (d.time.dst || 0) > 0;
      }

      // WiFi
      if (d.wifi) {
        state.wifi.state = d.wifi.state;
        state.wifi.name = d.wifi.name;
        updateWifiUI();
      }

      // Bells
      if (d.bells) {
        state.bells = d.bells;
        renderBells();
      }

      // Scheduler
      if (d.sched) {
        state.sched.ring = d.sched.ring;
        state.sched.ringId = d.sched.ringId;
        state.sched.nextH = d.sched.nextH;
        state.sched.nextM = d.sched.nextM;
        state.sched.nextT = d.sched.nextT;
        updateNextBellUI();
      }

      console.log('State loaded, connecting SSE...');
      connectSSE();
    }

    // Funzioni per operazioni CRUD
    async function loadBells() {
      const data = await api('/api/bells');
      if (data) {
        state.bells = data;
        renderBells();
      }
    }

    async function loadWifiInfo() {
      const data = await api('/api/wifi/status');
      if (data) {
        document.getElementById('wifiIP').textContent = data.ip || '-';
        document.getElementById('wifiSSID').textContent = data.ssid || '-';
        document.getElementById('wifiNTP').textContent = data.ntpSynced ? 'Sincronizzato' : 'Non sincronizzato';
        document.getElementById('newWifiSSID').value = data.ssid || '';

        const box = document.getElementById('wifiStatusBox');
        const detail = document.getElementById('wifiStatusDetail');
        box.className = 'wifi-status';

        if (data.state === 4) {
          box.classList.add('ap-mode');
          detail.textContent = 'Modalita Access Point';
        } else if (data.state === 3) {
          box.classList.add('connected');
          detail.textContent = 'Connesso e Sincronizzato';
        } else if (data.state === 2) {
          box.classList.add('connected');
          detail.textContent = 'Connesso (non sync)';
        } else {
          box.classList.add('disconnected');
          detail.textContent = 'Disconnesso';
        }
      }
    }

    async function loadTimezone() {
      const data = await api('/api/timezone');
      if (data) {
        document.getElementById('settingsTimezone').value = data.gmtOffset || 3600;
        document.getElementById('settingsDST').checked = data.dstOffset > 0;
      }
    }

    async function loadLog() {
      const data = await api('/api/log');
      const logDiv = document.getElementById('bellLog');
      if (data && data.length > 0) {
        logDiv.innerHTML = data.map(e =>
          `<div class="log-item">${e.timeStr} - Campanella ${e.bellId}</div>`
        ).join('');
      } else {
        logDiv.innerHTML = 'Nessun log';
      }
    }

    function updateWifiStatus(data) {
      const header = document.getElementById('wifiStatusHeader');
      const text = document.getElementById('wifiStatusText');
      let dotClass = 'status-disconnected';
      let statusText = 'Disconnesso';

      if (data.wifiState === 4) {
        dotClass = 'status-ap';
        statusText = 'AP Mode';
      } else if (data.wifiState === 3) {
        dotClass = 'status-synced';
        statusText = 'Sincronizzato';
      } else if (data.wifiState === 2) {
        dotClass = 'status-connected';
        statusText = 'Connesso';
      } else if (data.wifiState === 1) {
        dotClass = 'status-connecting';
        statusText = 'Connessione...';
      }

      header.innerHTML = `<span class="status-dot ${dotClass}"></span><span>${statusText}</span>`;
    }

    // === Render ===
    // Supporta sia formato lungo (/api/bells) che abbreviato (/api/state)
    function renderBells() {
      const tbody = document.getElementById('bellsTable');
      tbody.innerHTML = state.bells.map(b => {
        // Normalizza campi (supporta h/hour, m/minute, etc.)
        const hour = b.hour !== undefined ? b.hour : b.h;
        const min = b.minute !== undefined ? b.minute : b.m;
        const dur = b.duration !== undefined ? b.duration : b.d;
        const on = b.enabled !== undefined ? b.enabled : (b.on === 1);
        const type = b.type !== undefined ? b.type : b.t;
        return `
        <tr>
          <td>${String(hour).padStart(2,'0')}:${String(min).padStart(2,'0')}</td>
          <td>${getDaysStr(b.days)}</td>
          <td>${type}</td>
          <td>${dur}s</td>
          <td>
            <label class="switch">
              <input type="checkbox" ${on ? 'checked' : ''} onchange="toggleBell(${b.id}, this.checked)">
              <span class="slider"></span>
            </label>
          </td>
          <td class="actions">
            <button class="action-btn edit" onclick="editBell(${b.id})">&#9998;</button>
            <button class="action-btn delete" onclick="deleteBell(${b.id})">&#128465;</button>
          </td>
        </tr>
      `;
      }).join('');
    }

    function getDaysStr(days) {
      const names = ['Lun', 'Mar', 'Mer', 'Gio', 'Ven', 'Sab', 'Dom'];
      return names.filter((_, i) => days & (1 << i)).join(' ') || 'Nessuno';
    }

    // === Modal ===
    async function openModal(id) {
      document.getElementById(id).classList.add('active');
      if (id === 'settingsModal') {
        // Solo il log (timezone gia' caricato all'init)
        await loadLog();
      }
      if (id === 'wifiModal') {
        // Carica in sequenza
        await loadWifiInfo();
        // scanNetworks non ha await perche' fa polling interno
        scanNetworks();
      }
      if (id === 'addBellModal' && !editingId) {
        resetBellForm();
      }
    }

    function closeModal(id) {
      document.getElementById(id).classList.remove('active');
      if (id === 'addBellModal') {
        editingId = null;
        resetBellForm();
      }
    }

    function resetBellForm() {
      document.getElementById('bellModalTitle').textContent = 'Aggiungi Campanella';
      document.getElementById('editBellId').value = '';
      document.getElementById('bellTime').value = '';
      document.getElementById('bellDuration').value = '3';
      document.getElementById('bellType').value = '';
      document.querySelectorAll('.day-btn').forEach((btn, i) => {
        btn.classList.toggle('active', i < 5);
      });
    }

    // === Bell CRUD ===
    async function saveBell() {
      const time = document.getElementById('bellTime').value;
      const duration = parseInt(document.getElementById('bellDuration').value) || 3;
      const type = document.getElementById('bellType').value;

      if (!time || !type) {
        alert('Compila tutti i campi!');
        return;
      }

      const [hour, minute] = time.split(':').map(Number);
      let days = 0;
      document.querySelectorAll('.day-btn.active').forEach(btn => {
        days |= (1 << parseInt(btn.dataset.day));
      });

      if (days === 0) {
        alert('Seleziona almeno un giorno!');
        return;
      }

      const bellData = { hour, minute, duration, type, days, enabled: true };

      if (editingId) {
        await api('/api/bells/' + editingId, { method: 'PUT', body: JSON.stringify(bellData) });
      } else {
        await api('/api/bells', { method: 'POST', body: JSON.stringify(bellData) });
      }

      closeModal('addBellModal');
      await loadBells();
    }

    function editBell(id) {
      const bell = state.bells.find(b => b.id === id);
      if (!bell) return;

      // Normalizza campi (supporta formato lungo e abbreviato)
      const hour = bell.hour !== undefined ? bell.hour : bell.h;
      const min = bell.minute !== undefined ? bell.minute : bell.m;
      const dur = bell.duration !== undefined ? bell.duration : bell.d;
      const type = bell.type !== undefined ? bell.type : bell.t;

      editingId = id;
      document.getElementById('bellModalTitle').textContent = 'Modifica Campanella';
      document.getElementById('editBellId').value = id;
      document.getElementById('bellTime').value =
        String(hour).padStart(2,'0') + ':' + String(min).padStart(2,'0');
      document.getElementById('bellDuration').value = dur;
      document.getElementById('bellType').value = type;

      document.querySelectorAll('.day-btn').forEach(btn => {
        const day = parseInt(btn.dataset.day);
        btn.classList.toggle('active', !!(bell.days & (1 << day)));
      });

      openModal('addBellModal');
    }

    async function deleteBell(id) {
      if (!confirm('Eliminare questa campanella?')) return;
      await api('/api/bells/' + id, { method: 'DELETE' });
      await loadBells();
    }

    async function toggleBell(id, enabled) {
      await api('/api/bells/' + id, { method: 'PUT', body: JSON.stringify({ enabled }) });
    }

    // === Settings ===
    async function saveSettings() {
      const name = document.getElementById('settingsName').value;
      const globalEnabled = document.getElementById('settingsGlobal').checked;
      const gmtOffset = parseInt(document.getElementById('settingsTimezone').value);
      const dstOffset = document.getElementById('settingsDST').checked ? 3600 : 0;

      await api('/api/settings', {
        method: 'PUT',
        body: JSON.stringify({ institutionName: name, globalEnabled })
      });

      await api('/api/timezone', {
        method: 'POST',
        body: JSON.stringify({ gmtOffset, dstOffset })
      });

      closeModal('settingsModal');
      // Ricarica in sequenza
      await loadSettings();
      await loadStatus();
    }

    // === WiFi ===
    let scanRetries = 0;

    async function scanNetworks() {
      const btn = document.getElementById('scanBtn');
      const status = document.getElementById('scanStatus');
      const select = document.getElementById('wifiNetworkList');

      btn.disabled = true;
      btn.textContent = '...';
      status.textContent = 'Scansione in corso...';

      const data = await api('/api/wifi/scan');

      if (data && data.scanning) {
        // Scansione in corso, riprova tra 2 secondi
        scanRetries++;
        if (scanRetries < 5) {
          status.textContent = 'Scansione in corso...';
          setTimeout(scanNetworks, 2000);
          return;
        } else {
          status.textContent = 'Timeout scansione';
          btn.disabled = false;
          btn.textContent = 'Scansiona';
          scanRetries = 0;
          return;
        }
      }

      scanRetries = 0;
      btn.disabled = false;
      btn.textContent = 'Scansiona';

      if (data && Array.isArray(data) && data.length > 0) {
        select.innerHTML = '<option value="">-- Seleziona rete --</option>';
        data.forEach(net => {
          const signal = net.rssi > -50 ? '+++' : net.rssi > -70 ? '++' : '+';
          const lock = net.secure ? '[*]' : '';
          select.innerHTML += `<option value="${net.ssid}">${net.ssid} ${signal} ${lock}</option>`;
        });
        status.textContent = `Trovate ${data.length} reti`;
      } else {
        status.textContent = 'Nessuna rete trovata';
      }
    }

    function selectNetwork(ssid) {
      if (ssid) {
        document.getElementById('newWifiSSID').value = ssid;
      }
    }

    function togglePassword() {
      const input = document.getElementById('newWifiPass');
      const btn = document.getElementById('togglePassBtn');
      if (input.type === 'password') {
        input.type = 'text';
        btn.textContent = '🔒';
      } else {
        input.type = 'password';
        btn.textContent = '👁';
      }
    }

    async function saveWifi() {
      const ssid = document.getElementById('newWifiSSID').value;
      const password = document.getElementById('newWifiPass').value;

      if (!ssid) {
        alert('Inserisci SSID!');
        return;
      }

      // Richiesta prioritaria - bypassa il limite
      const result = await api('/api/wifi', {
        method: 'POST',
        body: JSON.stringify({ ssid, password }),
        priority: true
      });

      if (result && result.success) {
        alert('Configurazione salvata! Il dispositivo si riavviera...');
        closeModal('wifiModal');
      } else {
        alert('Errore nel salvataggio. Riprova.');
      }
    }

    // === Day Buttons ===
    document.querySelectorAll('.day-btn').forEach(btn => {
      btn.addEventListener('click', () => btn.classList.toggle('active'));
    });

    // === Init ===
    // GET /api/state (una sola chiamata), poi SSE delta events
    loadState();
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H

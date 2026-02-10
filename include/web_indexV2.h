#ifndef WEB_INDEX_H
#define WEB_INDEX_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<title>DAQ ESP32-S3 PRO V2</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
    body { font-family: 'Segoe UI', sans-serif; background: #121212; color: #e0e0e0; text-align: center; margin:0; }
    .header { background: #1f1f1f; padding: 20px; border-bottom: 2px solid #007bff; display: flex; justify-content: space-between; align-items: center; }
    .container { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; padding: 20px; }
    .card { background: #1e1e1e; border-radius: 12px; padding: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); border: 1px solid #333; transition: 0.3s; }
    .card:hover { border-color: #007bff; }
    .voltage { font-size: 32px; font-weight: bold; margin: 10px 0; }
    .btn { padding: 12px; border-radius: 8px; border: none; cursor: pointer; width: 100%; font-weight: bold; transition: 0.3s; }
    .on { background: #2e7d32; color: white; } .off { background: #c62828; color: white; }
    .edit-icon { cursor: pointer; font-size: 14px; margin-left: 8px; color: #007bff; }
    .footer { background: #1a1a1a; padding: 15px; font-size: 12px; color: #888; border-top: 1px solid #333; position: sticky; bottom: 0; }
    .stat-val { color: #00e5ff; font-weight: bold; }
</style>
</head>
<body>
    <div class="header">
        <span style="font-weight:bold; color:#007bff">DAQ v2.0</span>
        <h1 style="margin:0; font-size:20px;">Centrale de Surveillance 24V</h1>
        <div id="status-dot" style="width:12px; height:12px; border-radius:50%; background:#2e7d32"></div>
    </div>

    <div class="container" id="analog-grid"></div>
    <hr style="border:1px solid #333; margin:0 20px;">
    <div class="container" id="relay-grid"></div>

    <div class="footer">
        CPU: <span class="stat-val" id="cpu-temp">--</span>°C | 
        WiFi: <span class="stat-val" id="wifi-sig">--</span> dBm | 
        EEPROM: <span class="stat-val" id="eeprom-stat">--</span> | 
        Uptime: <span class="stat-val" id="uptime">--</span>s
    </div>

<script>
    function updateData() {
        fetch('/data').then(r => r.json()).then(d => {
            let aH = '';
            d.analog.forEach((v, i) => {
                let color = (d.thresholds[i] > 0 && v > d.thresholds[i]) ? "#ff5252" : "#00e5ff";
                aH += `<div class="card">
                    <div style="font-size:12px">${d.names[i]} <span class="edit-icon" onclick="rename(${i})">✏️</span></div>
                    <div class="voltage" style="color:${color}">${v.toFixed(2)}V</div>
                    <div style="font-size:11px; color:#666">Seuil: ${d.thresholds[i]}V <span class="edit-icon" onclick="setLimit(${i})">⚙️</span></div>
                </div>`;
            });
            document.getElementById('analog-grid').innerHTML = aH;

            let rH = '';
            d.relays.forEach((s, i) => {
                rH += `<div class="card"><small>Relais ${i+1}</small><br><br>
                <button class="btn ${s?'on':'off'}" onclick="tglR(${i},${s?0:1})">${s?'ACTIVE':'ETEINT'}</button></div>`;
            });
            document.getElementById('relay-grid').innerHTML = rH;
        });
    }

    function updateHealth() {
        fetch('/scan').then(r => r.json()).then(h => {
            document.getElementById('cpu-temp').innerText = h.cpu_temp.toFixed(1);
            document.getElementById('wifi-sig').innerText = h.wifi_rssi;
            document.getElementById('eeprom-stat').innerText = h.eeprom_status;
            document.getElementById('uptime').innerText = h.uptime_sec;
        });
    }

    function rename(id) { let n = prompt("Nom du capteur ?"); if(n) fetch(`/rename?id=${id}&name=${n}`).then(()=>updateData()); }
    function setLimit(id) { let l = prompt("Seuil alerte (Volts) ?"); if(l) fetch(`/limit?id=${id}&val=${l}`).then(()=>updateData()); }
    function tglR(id,s) { fetch(`/relay?id=${id}&state=${s}`).then(()=>updateData()); }
    
    setInterval(updateData, 1000);
    setInterval(updateHealth, 5000);
    updateData(); updateHealth();
</script>
</body></html>
)rawliteral";
#endif
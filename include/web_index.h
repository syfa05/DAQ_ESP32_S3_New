#ifndef WEB_INDEX_H
#define WEB_INDEX_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<title>DAQ S3 API V3</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
    body { font-family: 'Segoe UI', sans-serif; background: #121212; color: #e0e0e0; text-align: center; margin:0; }
    .header { background: #1f1f1f; padding: 15px; border-bottom: 2px solid #007bff; display: flex; justify-content: space-between; }
    .container { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 10px; padding: 20px; }
    .card { background: #1e1e1e; border-radius: 8px; padding: 15px; border: 1px solid #333; }
    .voltage { font-size: 28px; font-weight: bold; color: #00e5ff; }
    .btn { padding: 10px; border-radius: 5px; border: none; cursor: pointer; width: 100%; font-weight: bold; }
    .on { background: #2e7d32; color: white; } .off { background: #c62828; color: white; }
    .textarea { width: 90%; height: 200px; background: #000; color: #0f0; font-family: monospace; padding: 10px; border: 1px solid #007bff; }
    .footer { background: #1a1a1a; padding: 10px; font-size: 12px; color: #888; }
</style>
</head>
<body>
    <div class="header"><span>DAQ S3 PRO</span><div id="status">●</div></div>
    
    <h3>📊 Entrées & Relais</h3>
    <div class="container" id="analog-grid"></div>
    <div class="container" id="relay-grid"></div>

    <h3>🐍 Éditeur de Logique (API)</h3>
    <textarea id="codeEditor" placeholder="# Exemple: R1 = AN1 > 12.5"></textarea><br><br>
    <button class="btn on" style="width:200px" onclick="saveScript()">Enregistrer & Appliquer</button>
    <p id="msg"></p>

    <div class="card" style="grid-column: 1 / -1;">
        <h3>💾 Historique des données</h3>
        <button class="btn" style="background:#555; width:auto; padding:10px 30px;" onclick="downloadLogs()">
            Télécharger DATALOG.CSV
        </button>
        <button class="btn" style="background:#c62828; width:auto; margin-left:10px;" onclick="clearLogs()">
            Effacer les Logs
        </button>
    </div>

    <div class="footer">CPU: <span id="cpu">--</span>°C | WiFi: <span id="wifi">--</span>dBm | SD: <span id="sd">--</span></div>

<script>
    function refresh() {
        fetch('/data').then(r => r.json()).then(d => {
            let aH = ''; d.analog.forEach((v, i) => {
                aH += `<div class="card"><div>${d.names[i]}</div><div class="voltage">${v.toFixed(2)}V</div></div>`;
            });
            document.getElementById('analog-grid').innerHTML = aH;
            let rH = ''; d.relays.forEach((s, i) => {
                rH += `<div class="card">Relais ${i+1}<button class="btn ${s?'on':'off'}" onclick="tgl(${i},${s?0:1})">${s?'ON':'OFF'}</button></div>`;
            });
            document.getElementById('relay-grid').innerHTML = rH;
        });
        fetch('/scan').then(r => r.json()).then(h => {
            document.getElementById('cpu').innerText = h.cpu_temp;
            document.getElementById('wifi').innerText = h.wifi_rssi;
            document.getElementById('sd').innerText = h.sd_status;
        });
    }
    function tgl(id,s) { fetch(`/relay?id=${id}&state=${s}`).then(refresh); }
    function saveScript() {
        let formData = new FormData();
        formData.append("file", new Blob([document.getElementById('codeEditor').value]), "/script.txt");
        fetch('/upload', {method:'POST', body:formData}).then(() => alert("Script OK"));
    }

    function downloadLogs() {
        window.location.href = "/datalog.csv";
    }
    function clearLogs() {
        if(confirm("Effacer tout l'historique ?")) {
            fetch('/clear-logs').then(() => alert("Logs effacés"));
        }
    }
    fetch('/script.txt').then(r => r.text()).then(t => document.getElementById('codeEditor').value = t);
    setInterval(refresh, 2000); refresh();
</script>
</body></html>
)rawliteral";
#endif
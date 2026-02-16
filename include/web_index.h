#ifndef WEB_INDEX_H
#define WEB_INDEX_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
    <title>DAQ S3 PRO | Industrial Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        :root { --bg: #0f111a; --card: #1a1d2e; --primary: #007bff; --accent: #00e5ff; --text: #e0e6ed; --success: #28a745; --danger: #dc3545; }
        body { font-family: 'Segoe UI', Roboto, sans-serif; background: var(--bg); color: var(--text); margin: 0; }
        
        /* Navigation Sidebar */
        .sidebar { height: 100%; width: 220px; position: fixed; background: var(--card); padding-top: 20px; border-right: 1px solid #2d324d; }
        .sidebar a { padding: 15px 25px; text-decoration: none; font-size: 16px; color: #888; display: block; cursor: pointer; transition: 0.3s; }
        .sidebar a.active { color: var(--accent); background: rgba(0,229,255,0.05); border-left: 4px solid var(--accent); }
        .sidebar a:hover { color: #fff; }

        /* Main Content */
        .main { margin-left: 220px; padding: 30px; }
        .tab-content { display: none; }
        .tab-content.active { display: block; animation: fadeIn 0.4s; }
        @keyframes fadeIn { from {opacity: 0;} to {opacity: 1;} }

        /* Professional Cards */
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; }
        .card { background: var(--card); padding: 20px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.3); border: 1px solid #2d324d; }
        .card h3 { margin: 0 0 15px 0; font-size: 14px; color: #8b949e; text-transform: uppercase; letter-spacing: 1px; }
        .value { font-size: 32px; font-weight: bold; color: var(--accent); }

        /* Code Editor */
        #codeEditor { width: 100%; height: 300px; background: #050505; color: #50fa7b; border: 1px solid #2d324d; padding: 15px; border-radius: 8px; font-family: 'Consolas', monospace; resize: vertical; }
        .btn { background: var(--primary); color: white; border: none; padding: 12px 25px; border-radius: 6px; cursor: pointer; font-weight: 600; transition: 0.2s; }
        .btn:hover { filter: brightness(1.2); }
        .btn-install { background: var(--success); }

        /* Status Indicators */
        .status-bar { display: flex; gap: 20px; margin-bottom: 30px; font-size: 13px; color: #8b949e; }
        .status-item { display: flex; align-items: center; gap: 8px; }
        .dot { height: 10px; width: 10px; background: var(--success); border-radius: 50%; }
    </style>
</head>
<body>

<div class="sidebar">
    <div style="padding: 0 25px 30px; font-weight: bold; font-size: 20px; color: #fff;">DAQ S3 <span style="color:var(--accent)">PRO</span></div>
    <a onclick="openTab('monitor')" id="tab-monitor" class="active">📊 Monitoring</a>
    <a onclick="openTab('program')" id="tab-program">💻 Programmation C++</a>
    <a onclick="openTab('maintenance')" id="tab-maintenance">⚙️ Maintenance & PLC</a>
</div>

<div class="main">
    <div class="status-bar">
        <div class="status-item"><div class="dot"></div> System Live</div>
        <div class="status-item">CPU: <span id="cpu_temp">--</span>°C</div>
        <div class="status-item">RAM Mode: <span style="color:var(--accent)">Simulation</span></div>
    </div>

    <div id="monitor" class="tab-content active">
        <div class="grid" id="analog-grid"></div>
        <h2 style="margin-top:40px">Relais & Actionneurs</h2>
        <div class="grid" id="relay-grid"></div>
    </div>

    <div id="program" class="tab-content">
        <div class="card" style="max-width: 800px;">
            <h3>Éditeur de Logique (Runtime C++ TinyExpr)</h3>
            <p style="font-size:13px; color:#8b949e">Syntaxes admises : R1 = AN1 > 12.5 | R2 = (AN1 + AN2) / 2 < 10.0</p>
            <textarea id="codeEditor" spellcheck="false"></textarea>
            <div style="margin-top:15px;">
                <button class="btn" onclick="saveScript()">Enregistrer et Compiler</button>
            </div>
        </div>
    </div>

    <div id="maintenance" class="tab-content">
        <div class="grid">
            <div class="card">
                <h3>Installation OpenPLC</h3>
                <p style="font-size:13px">Bascule la carte en mode Automate Industriel (Ladder).</p>
                <button class="btn btn-install" onclick="alert('Lancement Web Serial API...')">Installer Runtime OpenPLC</button>
            </div>
            <div class="card">
                <h3>Firmware Update</h3>
                <p style="font-size:13px">Mettre à jour le logiciel de monitoring C++.</p>
                <input type="file" id="otaFile" style="display:none">
                <button class="btn" onclick="document.getElementById('otaFile').click()">Choisir .BIN</button>
            </div>
        </div>
    </div>
</div>

<script>
    function openTab(tabName) {
        document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.sidebar a').forEach(a => a.classList.remove('active'));
        document.getElementById(tabName).classList.add('active');
        document.getElementById('tab-' + tabName).classList.add('active');
    }

    function refresh() {
        fetch('/data').then(r => r.json()).then(d => {
            let aH = ''; d.analog.forEach((v, i) => {
                aH += `<div class="card"><h3>Entrée AN${i+1}</h3><div class="value">${v.toFixed(2)}<span style="font-size:16px">V</span></div></div>`;
            });
            document.getElementById('analog-grid').innerHTML = aH;
            
            let rH = ''; d.relays.forEach((s, i) => {
                rH += `<div class="card"><h3>Relais R${i+1}</h3><div style="color:${s? 'var(--accent)':'#444'}; font-weight:bold">${s?'ACTIVE':'VEILLE'}</div></div>`;
            });
            document.getElementById('relay-grid').innerHTML = rH;
            document.getElementById('cpu_temp').innerText = d.cpu;
        });
    }

    function saveScript() {
        let formData = new FormData();
        formData.append("file", new Blob([document.getElementById('codeEditor').value]), "/script.txt");
        fetch('/upload', {method:'POST', body:formData}).then(() => alert("Compilation et déploiement réussis !"));
    }

    fetch('/script.txt').then(r => r.text()).then(t => document.getElementById('codeEditor').value = t);
    setInterval(refresh, 2000); refresh();
</script>
</body></html>
)rawliteral";
#endif
#ifndef WEB_INDEX_H
#define WEB_INDEX_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3 DAQ Pro</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #1a1a1a; color: white; text-align: center; }
        .container { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; padding: 20px; }
        .card { background: #2d2d2d; border-radius: 12px; padding: 15px; border-bottom: 4px solid #007bff; }
        .voltage { font-size: 28px; font-weight: bold; color: #00ffcc; margin: 10px 0; }
        .relay-btn { padding: 10px 20px; border-radius: 20px; border: none; cursor: pointer; font-weight: bold; width: 100%; transition: 0.3s; }
        .on { background: #28a745; color: white; }
        .off { background: #dc3545; color: white; }
        h1 { color: #007bff; }
    </style>
</head>
<body>
    <h1>🎛️ DAQ System - 24V Monitoring</h1>
    <div class="container" id="analog-container"></div>
    <hr>
    <div class="container" id="relay-container"></div>

    <script>
        // Rafraîchissement des données analogiques (Toutes les 500ms pour plus de fluidité)
        setInterval(() => {
            fetch('/data').then(r => r.json()).then(d => {
                let html = '';
                d.analog.forEach((v, i) => {
                    let color = v > 24 ? "#ff4444" : (v > 12 ? "#00ffcc" : "#ffbb33");
                    html += `<div class="card">
                        <small>Entrée ${i+1}</small>
                        <div class="voltage" style="color:${color}">${v.toFixed(2)}V</div>
                    </div>`;
                });
                document.getElementById('analog-container').innerHTML = html;
            });
        }, 500);

        // Génération des boutons de relais (Une seule fois au chargement)
        let relayHtml = '';
        for(let i=0; i<8; i++) {
            relayHtml += `<div class="card">
                <small>Relais ${i+1}</small><br><br>
                <button id="btn-${i}" class="relay-btn off" onclick="toggleRelay(${i})">OFF</button>
            </div>`;
        }
        document.getElementById('relay-container').innerHTML = relayHtml;

        function toggleRelay(id) {
            let btn = document.getElementById('btn-' + id);
            let newState = btn.classList.contains('off') ? 1 : 0;
            fetch(`/relay?id=${id}&state=${newState}`).then(() => {
                btn.classList.toggle('on');
                btn.classList.toggle('off');
                btn.innerText = newState ? 'ON' : 'OFF';
            });
        }
    </script>
</body>
</html>
)rawliteral";

#endif
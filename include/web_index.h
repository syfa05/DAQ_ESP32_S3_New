#ifndef WEB_INDEX_H
#define WEB_INDEX_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<title>DAQ S3 PRO | Industrial Controller</title>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>
:root { --bg:#0f111a; --card:#1a1d2e; --primary:#007bff;
--accent:#00e5ff; --text:#e0e6ed; --success:#28a745; --danger:#dc3545; }

body { font-family:'Segoe UI',Roboto,sans-serif;background:var(--bg);
color:var(--text);margin:0;}

.sidebar {height:100%;width:220px;position:fixed;background:var(--card);
padding-top:20px;border-right:1px solid #2d324d;}

.sidebar a {padding:15px 25px;text-decoration:none;font-size:16px;
color:#888;display:block;cursor:pointer;}

.sidebar a.active {color:var(--accent);
background:rgba(0,229,255,0.05);
border-left:4px solid var(--accent);}

.main {margin-left:220px;padding:30px;}
.tab-content {display:none;}
.tab-content.active {display:block;}

.grid {display:grid;
grid-template-columns:repeat(auto-fit,minmax(200px,1fr));
gap:20px;}

.card {background:var(--card);padding:20px;border-radius:12px;
border:1px solid #2d324d;}

.card h3 {margin:0 0 15px 0;font-size:14px;color:#8b949e;
text-transform:uppercase;}

.value {font-size:32px;font-weight:bold;color:var(--accent);}

.btn {background:var(--primary);color:white;border:none;
padding:12px 25px;border-radius:6px;cursor:pointer;}

.btn-install {background:var(--success);}

.status-bar {display:flex;gap:20px;margin-bottom:30px;font-size:13px;color:#8b949e;}
.dot {height:10px;width:10px;background:var(--success);border-radius:50%;}

/* Console série */
#serialConsole {
height:150px;background:#000;color:#0f0;font-family:monospace;
padding:10px;overflow-y:auto;border-radius:5px;margin-bottom:10px;
font-size:12px;
}
</style>
</head>

<body>

<div class="sidebar">
<div style="padding:0 25px 30px;font-weight:bold;font-size:20px;color:#fff;">
DAQ S3 <span style="color:var(--accent)">PRO</span>
</div>
<a onclick="openTab('monitor')" id="tab-monitor" class="active">📊 Monitoring</a>
<a onclick="openTab('program')" id="tab-program">💻 Programmation</a>
<a onclick="openTab('maintenance')" id="tab-maintenance">⚙️ Maintenance</a>
</div>

<div class="main">

<div class="status-bar">
<div class="dot"></div> System Live
CPU: <span id="cpu_temp">--</span>°C
</div>

<!-- MONITOR -->
<div id="monitor" class="tab-content active">
<div class="grid" id="analog-grid"></div>
<h2 style="margin-top:40px">Relais</h2>
<div class="grid" id="relay-grid"></div>
</div>

<!-- PROGRAM -->
<div id="program" class="tab-content">
<div class="card" style="max-width:800px;">
<h3>Éditeur Logique</h3>
<textarea id="codeEditor" style="width:100%;height:300px;"></textarea>
<button class="btn" onclick="saveScript()">Compiler</button>
</div>
</div>

<!-- MAINTENANCE -->
<div id="maintenance" class="tab-content">
<div class="grid">

<div class="card" style="grid-column:1 / -1;">
<h3>Console Série Directe (Web Serial)</h3>

<div id="serialConsole">
Connectez votre carte en USB puis cliquez sur Connecter.
</div>

<button class="btn" id="btnConnect"
onclick="connectSerial()">Connecter USB</button>

<button class="btn btn-install"
onclick="installPLC()" style="margin-left:10px">
Installer Runtime OpenPLC
</button>
</div>

<div class="card">
<h3>Firmware Update</h3>
<input type="file" id="otaFile" style="display:none">
<button class="btn"
onclick="document.getElementById('otaFile').click()">
Choisir .BIN
</button>
</div>

<div class="card">
<h3>Diagnostic</h3>
<button class="btn"
style="background:var(--danger)"
onclick="rebootCard()">Hard Reboot</button>
</div>

</div>
</div>

</div>

<script>

/* ---------- Navigation ---------- */
function openTab(tabName){
document.querySelectorAll('.tab-content')
.forEach(t=>t.classList.remove('active'));
document.querySelectorAll('.sidebar a')
.forEach(a=>a.classList.remove('active'));

document.getElementById(tabName).classList.add('active');
document.getElementById('tab-'+tabName).classList.add('active');
}

/* ---------- Monitoring ---------- */
function refresh(){
fetch('/data').then(r=>r.json()).then(d=>{
let aH='';
d.analog.forEach((v,i)=>{
aH+=`<div class="card">
<h3>Entrée AN${i+1}</h3>
<div class="value">${v.toFixed(2)}V</div></div>`;
});
document.getElementById('analog-grid').innerHTML=aH;

let rH='';
d.relays.forEach((s,i)=>{
rH+=`<div class="card">
<h3>Relais R${i+1}</h3>
<div style="font-weight:bold;color:${s?'var(--accent)':'#444'}">
${s?'ACTIVE':'OFF'}
</div></div>`;
});
document.getElementById('relay-grid').innerHTML=rH;

document.getElementById('cpu_temp').innerText=d.cpu;
});
}

setInterval(refresh,2000); refresh();

/* ---------- Script ---------- */
function saveScript(){
let fd=new FormData();
fd.append("file",
new Blob([document.getElementById('codeEditor').value]),
"/script.txt");

fetch('/upload',{method:'POST',body:fd})
.then(()=>alert("Script envoyé"));
}

/* ---------- Web Serial ---------- */
let port;
let reader;

async function connectSerial(){
try{
port=await navigator.serial.requestPort();
await port.open({baudRate:115200});

document.getElementById('btnConnect').innerText="Connecté";

const decoder=new TextDecoderStream();
port.readable.pipeTo(decoder.writable);
reader=decoder.readable.getReader();

while(true){
const {value,done}=await reader.read();
if(done) break;

let div=document.getElementById('serialConsole');
div.innerHTML+=value;
div.scrollTop=div.scrollHeight;
}
}catch(e){
alert(e);
}
}

function installPLC(){
if(!port) return alert("Connectez USB d'abord");
alert("Prêt pour flash OpenPLC");
}

function rebootCard(){
fetch('/reboot');
}

</script>

</body></html>
)rawliteral";

#endif

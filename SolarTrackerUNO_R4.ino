#include <WiFiS3.h>
#include <Servo.h>
#include <math.h>

// Arduino UNO R4 WiFi dual-axis tracker + standalone access point.
const char AP_SSID[] = "solar tracker";
const char AP_PASS[] = "password"; // WPA2 requires at least 8 characters
WiFiServer server(80);
Servo horizontalServo, verticalServo;
const uint8_t LDR_PINS[4] = {A0, A1, A2, A3}; // TL TR BL BR
const uint8_t H_PIN = 9, V_PIN = 10;
int sensor[4] = {0, 0, 0, 0};
int hAngle = 90, vAngle = 90;
const int MIN_ANGLE = 15, MAX_ANGLE = 165;
const int DEADBAND = 65; // analogRead 0..1023 on UNO R4 by default
const unsigned long TRACK_INTERVAL = 250;
unsigned long lastTrack = 0;

// Small standalone HTML page. No CDN, internet, or additional files needed.
const char PAGE[] = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Solar Tracker</title><style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,Segoe UI,sans-serif}body{margin:0;background:#091426;color:#eaf2ff}main{max-width:760px;margin:auto;padding:22px}h1{margin:0 0 6px}p,.sub{color:#a7bad2}.card{background:#15253c;border:1px solid #30455f;border-radius:18px;padding:18px;margin:15px 0}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.metric{background:#20344d;padding:13px;border-radius:12px}.metric b{display:block;font-size:1.25rem;margin-top:5px}.small{font-size:.82rem;color:#afc0d7}svg{width:100%;height:auto}label{display:block;margin:10px 0 5px}input{width:100%;box-sizing:border-box;border:1px solid #647b98;border-radius:8px;background:#0e1b2c;color:white;padding:10px;font-size:1rem}button{background:#f9b934;border:0;border-radius:10px;padding:11px 16px;font-weight:700;cursor:pointer;margin-top:12px}.sun{filter:drop-shadow(0 0 12px #ffbe43)}.status{color:#9fe1a6}#clock{font-size:1.6rem;font-weight:700}#error{color:#ffb0a8} @media(max-width:430px){main{padding:12px}.card{padding:12px}}
</style></head><body><main><h1>☀ Solar Tracker</h1><div class="sub">Arduino UNO R4 WiFi · Standalone dashboard</div>
<div class="card"><div class="small">Local time</div><div id="clock">--:--:--</div><svg viewBox="0 0 640 280" role="img" aria-label="Sunrise to sunset sun path">
<path d="M45 230 Q320 -160 595 230" stroke="#f4b94b" stroke-width="3" stroke-dasharray="7 7" fill="none"/>
<line x1="20" y1="230" x2="620" y2="230" stroke="#71839e" stroke-width="2"/>
<g id="sun" class="sun" transform="translate(320 35)"><circle r="18" fill="#ffd35c"/><g stroke="#ffd35c" stroke-width="3"><path d="M0 -28v-9M0 28v9M-28 0h-9M28 0h9M-20 -20l-7 -7M20 20l7 7M-20 20l-7 7M20 -20l7 -7"/></g></g>
<text x="45" y="255" fill="#c3d1e2" text-anchor="middle">Sunrise</text><text x="595" y="255" fill="#c3d1e2" text-anchor="middle">Sunset</text>
<text x="320" y="273" fill="#d3e0ee" text-anchor="middle" id="sunLabel">--</text></svg>
<div class="grid"><div class="metric"><span class="small">Sunrise</span><b id="rise">--</b></div><div class="metric"><span class="small">Sunset</span><b id="set">--</b></div><div class="metric"><span class="small">Solar altitude</span><b id="alt">--</b></div><div class="metric"><span class="small">Solar azimuth</span><b id="az">--</b></div></div>
<p class="small" id="night">Sun path is an illustrative daily arc; the altitude and azimuth are calculated from your coordinates.</p></div>
<div class="card"><h2>Tracker readings</h2><div id="status" class="status">Connecting...</div><div class="grid">
<div class="metric"><span class="small">Horizontal servo</span><b id="h">--</b></div><div class="metric"><span class="small">Vertical servo</span><b id="v">--</b></div>
<div class="metric"><span class="small">Top left</span><b id="tl">--</b></div><div class="metric"><span class="small">Top right</span><b id="tr">--</b></div>
<div class="metric"><span class="small">Bottom left</span><b id="bl">--</b></div><div class="metric"><span class="small">Bottom right</span><b id="br">--</b></div></div></div>
<div class="card"><h2>Sun clock location</h2><p class="small">Enter the tracker location in decimal degrees. Values are saved in this browser. The clock uses this device's local time zone.</p>
<label>Latitude (north positive)</label><input id="lat" type="number" min="-90" max="90" step="any" value="34.98">
<label>Longitude (east positive)</label><input id="lon" type="number" min="-180" max="180" step="any" value="-101.92">
<button id="save">Save location</button><p id="error"></p><p class="small">Default coordinates: Canyon, Texas. If viewing from a different time zone, your phone's time zone must match the tracker's location for sunrise and sunset clock labels to be correct.</p></div>
</main><script>
const $=id=>document.getElementById(id);
const rad=Math.PI/180, deg=180/Math.PI;
const fmt=d=>d.toLocaleTimeString('en-US',{hour:'numeric',minute:'2-digit',hour12:true});
const fmtFull=d=>d.toLocaleTimeString('en-US',{hour:'numeric',minute:'2-digit',second:'2-digit',hour12:true});
$('lat').value=localStorage.getItem('solar_lat')??'34.98';
$('lon').value=localStorage.getItem('solar_lon')??'-101.92';
$('save').onclick=()=>{let a=Number($('lat').value),b=Number($('lon').value);if(!Number.isFinite(a)||!Number.isFinite(b)||a< -90||a>90||b< -180||b>180||!$('lat').value||!$('lon').value){$('error').textContent='Enter a valid latitude and longitude.';return;}localStorage.setItem('solar_lat',a);localStorage.setItem('solar_lon',b);$('error').textContent='Location saved.';updateClock();};
// NOAA-style approximate solar position. UTC Julian date, local wall-clock sunrise/sunset.
function solar(d,lat,lon){
 const jd=d.getTime()/86400000+2440587.5, T=(jd-2451545)/36525;
 const L=(280.46646+T*(36000.76983+0.0003032*T))%360;
 const M=(357.52911+T*(35999.05029-0.0001537*T))*rad;
 const ecc=0.016708634-T*(0.000042037+0.0000001267*T);
 const C=Math.sin(M)*(1.914602-T*(0.004817+0.000014*T))+Math.sin(2*M)*(0.019993-0.000101*T)+Math.sin(3*M)*0.000289;
 const omega=(125.04-1934.136*T)*rad;
 const lambda=(L+C-0.00569-0.00478*Math.sin(omega))*rad;
 const eps=(23+(26+(21.448-T*(46.815+T*(0.00059-T*0.001813)))/60)/60+0.00256*Math.cos(omega))*rad;
 const dec=Math.asin(Math.sin(eps)*Math.sin(lambda));
 const y=Math.tan(eps/2)**2;
 const eq=4*deg*(y*Math.sin(2*L*rad)-2*ecc*Math.sin(M)+4*ecc*y*Math.sin(M)*Math.cos(2*L*rad)-0.5*y*y*Math.sin(4*L*rad)-1.25*ecc*ecc*Math.sin(2*M));
 const utcMin=d.getUTCHours()*60+d.getUTCMinutes()+d.getUTCSeconds()/60;
 let tst=((utcMin+eq+4*lon)%1440+1440)%1440;
 const hourAngle=(tst/4-180)*rad;
 const phi=lat*rad;
 const sinAlt=Math.sin(phi)*Math.sin(dec)+Math.cos(phi)*Math.cos(dec)*Math.cos(hourAngle);
 const alt=Math.asin(Math.max(-1,Math.min(1,sinAlt)));
 const az=(Math.atan2(Math.sin(hourAngle),Math.cos(hourAngle)*Math.sin(phi)-Math.tan(dec)*Math.cos(phi))*deg+180+360)%360;
 return {alt:alt*deg,az,eq,dec};
}
function updateClock(){
 const now=new Date(),lat=Number($('lat').value),lon=Number($('lon').value);
 $('clock').textContent=fmtFull(now);
 if(!Number.isFinite(lat)||!Number.isFinite(lon)||Math.abs(lat)>90||Math.abs(lon)>180)return;
 const s=solar(now,lat,lon);
 $('alt').textContent=s.alt.toFixed(1)+'°';$('az').textContent=s.az.toFixed(1)+'°';
 // Sunrise/sunset from declination and equation of time at local noon (approximation).
 const noon=new Date(now.getFullYear(),now.getMonth(),now.getDate(),12);
 const n=solar(noon,lat,lon),phi=lat*rad;
 const cosH=(Math.cos(90.833*rad)/(Math.cos(phi)*Math.cos(n.dec)))-Math.tan(phi)*Math.tan(n.dec);
 if(cosH < -1 || cosH > 1){$('rise').textContent=cosH< -1?'No sunrise':'No sunrise';$('set').textContent=cosH< -1?'No sunset':'No sunset';$('sun').style.display='none';$('sunLabel').textContent='Polar day/night';return;}
 const H=Math.acos(cosH)*deg;
 // UTC minutes since UTC midnight of the local date; account for time zone offset.
 const offset=-noon.getTimezoneOffset();
 const solarNoon=720-4*lon-n.eq+offset;
 const midnight=new Date(now.getFullYear(),now.getMonth(),now.getDate());
 const rise=new Date(midnight.getTime()+(solarNoon-4*H)*60000);
 const set=new Date(midnight.getTime()+(solarNoon+4*H)*60000);
 $('rise').textContent=fmt(rise);$('set').textContent=fmt(set);
 const fraction=(now-rise)/(set-rise);
 $('sun').style.display=(fraction>=0&&fraction<=1)?'':'none';
 if(fraction>=0&&fraction<=1){const x=45+550*fraction,y=230-390*fraction*(1-fraction);$('sun').setAttribute('transform',`translate(${x} ${y})`);$('sunLabel').textContent=fmt(now);}
 else $('sunLabel').textContent=fraction<0?'Before sunrise':'After sunset';
}
async function readings(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);const d=await r.json();$('status').textContent='● Live tracker';for(const k of ['h','v','tl','tr','bl','br'])$(k).textContent=d[k]+(k==='h'||k==='v'?'°':'');}catch(e){$('status').textContent='Tracker connection unavailable';}}
updateClock();readings();setInterval(updateClock,1000);setInterval(readings,1000);
</script></body></html>)HTML";

void sendPage(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print(PAGE);
}
void sendStatus(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print("{\"h\":"); client.print(hAngle);
  client.print(",\"v\":"); client.print(vAngle);
  client.print(",\"tl\":"); client.print(sensor[0]);
  client.print(",\"tr\":"); client.print(sensor[1]);
  client.print(",\"bl\":"); client.print(sensor[2]);
  client.print(",\"br\":"); client.print(sensor[3]);
  client.println("}");
}
void serviceHttp() {
  WiFiClient client = server.available();
  if (!client) return;
  client.setTimeout(200);
  String line = client.readStringUntil('\n');
  line.trim();
  // Drain headers with bounded time and memory.
  unsigned long start = millis();
  while (client.connected() && millis()-start < 500) {
    if (!client.available()) continue;
    String h = client.readStringUntil('\n'); h.trim();
    if (h.length()==0) break;
  }
  if (line.startsWith("GET /api/status ")) sendStatus(client);
  else if (line.startsWith("GET / ") || line.startsWith("GET /?")) sendPage(client);
  else { client.println("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nNot found"); }
  client.flush(); client.stop();
}
void updateTracker() {
  if (millis()-lastTrack < TRACK_INTERVAL) return;
  lastTrack=millis();
  for (int i=0;i<4;i++) sensor[i]=analogRead(LDR_PINS[i]);
  int left=(sensor[0]+sensor[2])/2;
  int right=(sensor[1]+sensor[3])/2;
  int top=(sensor[0]+sensor[1])/2;
  int bottom=(sensor[2]+sensor[3])/2;
  if (left-right>DEADBAND) hAngle++;
  else if (right-left>DEADBAND) hAngle--;
  if (top-bottom>DEADBAND) vAngle++;
  else if (bottom-top>DEADBAND) vAngle--;
  hAngle=constrain(hAngle,MIN_ANGLE,MAX_ANGLE);
  vAngle=constrain(vAngle,MIN_ANGLE,MAX_ANGLE);
  horizontalServo.write(hAngle);
  verticalServo.write(vAngle);
}
void setup() {
  Serial.begin(115200);
  analogReadResolution(10);
  horizontalServo.attach(H_PIN); verticalServo.attach(V_PIN);
  horizontalServo.write(hAngle); verticalServo.write(vAngle);
  if (WiFi.status()==WL_NO_MODULE) {
    Serial.println("Wi-Fi module unavailable. Check board and firmware.");
    while(true) delay(1000);
  }
  Serial.println("Starting solar tracker AP...");
  int status=WiFi.beginAP(AP_SSID,AP_PASS);
  if (status!=WL_AP_LISTENING) {
    Serial.print("AP start failed, status="); Serial.println(status);
    while(true) delay(1000);
  }
  delay(1500);
  server.begin();
  Serial.print("SSID: ");Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");Serial.println(WiFi.localIP());
}
void loop() { updateTracker(); serviceHttp(); }

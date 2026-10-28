#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

const char* WIFI_SSID = "your ssid";
const char* WIFI_PASS = "your wifi password";

IPAddress local_IP(192, 168, xxx, xxx);
IPAddress gateway(192, 168, xxx, xxx);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(1, 1, 1, 1);
IPAddress dns2(1, 0, 0, 1);

const char* DEVICE_NAME = "Sensor name";
const char* HOST_NAME   = "esp8266-sensor-name";

#define I2C_SDA 4   // GPIO4 = D2
#define I2C_SCL 5   // GPIO5 = D1

const float TEMP_OFFSET  = 0.0;   // °C
const float HUM_OFFSET   = 0.0;   // %
const float PRESS_OFFSET = 0.0;   // hPa

const unsigned long SENSOR_INTERVAL_MS = 5000;
const uint16_t HISTORY_LEN = 120;

ESP8266WebServer server(80);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

bool ahtPresent = false;
bool bmpPresent = false;

float tempCantina  = NAN;
float humCantina   = NAN;
float pressCantina = NAN;

unsigned long lastSensorRead = 0;

float histTemp[HISTORY_LEN];
float histHum[HISTORY_LEN];
float histPress[HISTORY_LEN];
uint16_t histIndex = 0;
uint16_t histCount = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cellar</title>
<style>
:root{
  --bg:#0b1220;--panel:#111827;--card:#1e293b;--line:rgba(148,163,184,.14);
  --text:#e5eefb;--muted:#94a3b8;--ok:#22c55e;--bad:#ef4444;
}
*{box-sizing:border-box}
body{
  margin:0;font-family:Arial,Helvetica,sans-serif;
  background:linear-gradient(180deg,#0b1220,#111827);color:var(--text)
}
.wrap{max-width:1180px;margin:auto;padding:18px}
.hero,.card,.chart{
  background:linear-gradient(180deg,rgba(30,41,59,.96),rgba(15,23,42,.96));
  border:1px solid var(--line);border-radius:20px;box-shadow:0 10px 28px rgba(0,0,0,.28)
}
.hero{
  padding:18px 20px;margin-bottom:18px;display:flex;justify-content:space-between;
  gap:12px;align-items:center;flex-wrap:wrap
}
.grid,.charts,.bottom{display:grid;gap:14px}
.grid{grid-template-columns:repeat(auto-fit,minmax(220px,1fr));margin-bottom:18px}
.charts{grid-template-columns:repeat(auto-fit,minmax(360px,1fr));margin-bottom:18px}
.bottom{grid-template-columns:1fr 1fr}
@media(max-width:900px){.bottom,.charts{grid-template-columns:1fr}}
.card,.chart{padding:16px}
.label{font-size:14px;color:var(--muted);margin-bottom:8px}
.value{font-size:34px;font-weight:800}
.small,.meta{font-size:12px;color:var(--muted)}
canvas{
  width:100%;height:220px;background:#0b1220;border-radius:14px;
  border:1px solid rgba(148,163,184,.08)
}
.status{
  display:inline-block;margin-top:10px;padding:5px 10px;border-radius:999px;
  font-size:12px;font-weight:700
}
.ok{background:rgba(34,197,94,.14);color:#86efac}
.bad{background:rgba(239,68,68,.14);color:#fca5a5}
.links a{
  display:inline-block;margin:0 10px 10px 0;padding:10px 14px;border-radius:12px;
  text-decoration:none;color:#dffaff;background:rgba(34,211,238,.10);
  border:1px solid rgba(34,211,238,.18)
}
</style></head><body><div class="wrap">
<div class="hero">
  <div>
    <h1 style="margin:0">Cellar</h1>
    <div class="small">AHT20 + BMP280</div>
  </div>
  <div id="ip" class="small">Waiting for data...</div>
</div>

<div class="grid">
  <div class="card">
    <div class="label">Cellar temperature</div>
    <div class="value" id="temp">--</div>
    <div id="ahtStatus" class="status bad">AHT20 missing</div>
  </div>
  <div class="card">
    <div class="label">Cellar humidity</div>
    <div class="value" id="hum">--</div>
    <div class="small">AHT20 sensor</div>
  </div>
  <div class="card">
    <div class="label">Cellar pressure</div>
    <div class="value" id="press">--</div>
    <div id="bmpStatus" class="status bad">BMP280 missing</div>
  </div>
</div>

<div class="charts">
  <div class="chart"><div class="label">Cellar temperature</div><div class="meta" id="m1">--</div><canvas id="c1"></canvas></div>
  <div class="chart"><div class="label">Cellar humidity</div><div class="meta" id="m2">--</div><canvas id="c2"></canvas></div>
  <div class="chart"><div class="label">Cellar pressure</div><div class="meta" id="m3">--</div><canvas id="c3"></canvas></div>
</div>

<div class="bottom">
  <div class="card">
    <div class="label">Wiring</div>
    <div class="small">
      GPIO5 → SCL<br>
      GPIO4 → SDA<br>
      3.3V → VCC<br>
      GND → GND
    </div>
  </div>
  <div class="card">
    <div class="label">Endpoint</div>
    <div class="links">
      <a href="/api/data" target="_blank">/api/data</a>
      <a href="/api/ha" target="_blank">/api/ha</a>
    </div>
    <div class="small">
      Samples: <span id="count">--</span><br>
      Uptime: <span id="up">--</span>
    </div>
  </div>
</div>
</div>

<script>
function fmt(v,u,d=1){return(v===null||Number.isNaN(v))?"--":Number(v).toFixed(d)+" "+u}
function status(id,ok,t1,t0){
  const e=document.getElementById(id);
  e.className="status "+(ok?"ok":"bad");
  e.textContent=ok?t1:t0;
}
function stats(a){
  const v=a.filter(x=>x!==null&&!Number.isNaN(x));
  if(!v.length)return null;
  return{min:Math.min(...v),max:Math.max(...v),last:v[v.length-1]};
}
function meta(id,a,u){
  const s=stats(a);
  document.getElementById(id).textContent=s?("min "+s.min.toFixed(1)+" "+u+" · max "+s.max.toFixed(1)+" "+u+" · now "+s.last.toFixed(1)+" "+u):"no data";
}
function grad(ctx,h,a,b){
  const g=ctx.createLinearGradient(0,0,0,h);
  g.addColorStop(0,a); g.addColorStop(1,b); return g;
}
function line(ctx,p){
  if(p.length<2)return;
  ctx.beginPath();
  ctx.moveTo(p[0].x,p[0].y);
  for(let i=0;i<p.length-1;i++){
    const a=p[i],b=p[i+1],mx=(a.x+b.x)/2,my=(a.y+b.y)/2;
    ctx.quadraticCurveTo(a.x,a.y,mx,my);
  }
  ctx.lineTo(p[p.length-1].x,p[p.length-1].y);
}
function draw(id,data,unit,color,fill1,fill2){
  const c=document.getElementById(id),ctx=c.getContext("2d"),w=c.clientWidth,h=c.clientHeight;
  c.width=w;c.height=h;
  ctx.clearRect(0,0,w,h);
  const L=56,R=16,T=16,B=30,PW=w-L-R,PH=h-T-B;

  ctx.strokeStyle="rgba(148,163,184,.12)";
  for(let i=0;i<=4;i++){
    const y=T+PH/4*i;
    ctx.beginPath(); ctx.moveTo(L,y); ctx.lineTo(L+PW,y); ctx.stroke();
  }

  const vals=data.filter(v=>v!==null&&!Number.isNaN(v));
  if(!vals.length){
    ctx.fillStyle="#94a3b8";
    ctx.font="14px Arial";
    ctx.fillText("No data",L,T+24);
    return;
  }

  let mn=Math.min(...vals),mx=Math.max(...vals),pad=(mx-mn)*0.12;
  if(pad<0.5)pad=0.5;
  mn-=pad; mx+=pad;

  ctx.fillStyle="#94a3b8";
  ctx.font="12px Arial";
  for(let i=0;i<=4;i++){
    const y=T+PH/4*i,v=mx-((mx-mn)/4)*i;
    ctx.fillText(v.toFixed(1)+" "+unit,5,y+4);
  }

  const pts=[];
  for(let i=0;i<data.length;i++){
    const v=data[i];
    if(v===null||Number.isNaN(v))continue;
    pts.push({x:L+i*PW/Math.max(1,data.length-1),y:T+((mx-v)/(mx-mn))*PH});
  }

  if(pts.length===1){
    ctx.fillStyle=color;
    ctx.beginPath();
    ctx.arc(pts[0].x,pts[0].y,3,0,Math.PI*2);
    ctx.fill();
    return;
  }

  ctx.save();
  ctx.beginPath();
  ctx.moveTo(pts[0].x,T+PH);
  for(let i=0;i<pts.length-1;i++){
    const a=pts[i],b=pts[i+1],mx=(a.x+b.x)/2,my=(a.y+b.y)/2;
    ctx.quadraticCurveTo(a.x,a.y,mx,my);
  }
  ctx.lineTo(pts[pts.length-1].x,pts[pts.length-1].y);
  ctx.lineTo(pts[pts.length-1].x,T+PH);
  ctx.closePath();
  ctx.fillStyle=grad(ctx,h,fill1,fill2);
  ctx.fill();
  ctx.restore();

  ctx.strokeStyle=color;
  ctx.lineWidth=2.5;
  ctx.lineCap="round";
  ctx.lineJoin="round";
  line(ctx,pts);
  ctx.stroke();

  ctx.fillStyle=color;
  ctx.beginPath();
  ctx.arc(pts[pts.length-1].x,pts[pts.length-1].y,4,0,Math.PI*2);
  ctx.fill();
}
async function upd(){
  try{
    const r=await fetch("/api/data?_="+Date.now(),{cache:"no-store"});
    const j=await r.json();

    temp.textContent=fmt(j.temperature_cantina,"°C");
    hum.textContent=fmt(j.humidity_cantina,"%");
    press.textContent=fmt(j.pressure_cantina,"hPa");

    status("ahtStatus",j.aht_ok,"AHT20 ok","AHT20 missing/error");
    status("bmpStatus",j.bmp_ok,"BMP280 ok","BMP280 missing/error");

    draw("c1",j.hist.temperature_cantina,"°C","#22d3ee","rgba(34,211,238,.22)","rgba(34,211,238,.02)");
    draw("c2",j.hist.humidity_cantina,"%","#2dd4bf","rgba(45,212,191,.22)","rgba(45,212,191,.02)");
    draw("c3",j.hist.pressure_cantina,"hPa","#60a5fa","rgba(96,165,250,.22)","rgba(96,165,250,.02)");

    meta("m1",j.hist.temperature_cantina,"°C");
    meta("m2",j.hist.humidity_cantina,"%");
    meta("m3",j.hist.pressure_cantina,"hPa");

    document.getElementById("ip").textContent="IP: "+j.ip;
    document.getElementById("count").textContent=j.hist_count;
    document.getElementById("up").textContent=j.uptime_s+" s";
  }catch(e){
    document.getElementById("ip").textContent="Error reading data";
  }
}
upd();
setInterval(upd,5000);
window.addEventListener("resize",upd);
</script>
</body></html>
)rawliteral";

String ipToString(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

String jsonFloat(float v, uint8_t decimals = 1) {
  if (isnan(v)) return "null";
  return String(v, decimals);
}

String historyArrayToJson(float* arr, uint8_t decimals = 1) {
  String s;
  s.reserve(HISTORY_LEN * 8);
  s += "[";
  for (uint16_t i = 0; i < histCount; i++) {
    uint16_t idx = (histIndex + HISTORY_LEN - histCount + i) % HISTORY_LEN;
    if (i > 0) s += ",";
    if (isnan(arr[idx])) s += "null";
    else s += String(arr[idx], decimals);
  }
  s += "]";
  return s;
}

void addHistory(float t, float h, float p) {
  histTemp[histIndex]  = t;
  histHum[histIndex]   = h;
  histPress[histIndex] = p;

  histIndex = (histIndex + 1) % HISTORY_LEN;
  if (histCount < HISTORY_LEN) histCount++;
}

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOST_NAME);

  if (!WiFi.config(local_IP, gateway, subnet, dns1, dns2)) {
    Serial.println("Static IP configuration error");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void initSensors() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  ahtPresent = aht.begin();
  if (ahtPresent) {
    Serial.println("AHT20 detected.");
  } else {
    Serial.println("AHT20 NOT detected.");
  }

  bmpPresent = bmp.begin(0x76);
  if (!bmpPresent) bmpPresent = bmp.begin(0x77);

  if (bmpPresent) {
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500
    );
    Serial.println("BMP280 detected.");
  } else {
    Serial.println("BMP280 NOT detected.");
  }
}

void readSensors() {
  if (ahtPresent) {
    sensors_event_t humEvent, tempEvent;
    aht.getEvent(&humEvent, &tempEvent);

    if (!isnan(tempEvent.temperature) && !isnan(humEvent.relative_humidity)) {
      tempCantina = tempEvent.temperature + TEMP_OFFSET;
      humCantina  = humEvent.relative_humidity + HUM_OFFSET;
    } else {
      tempCantina = NAN;
      humCantina  = NAN;
    }
  } else {
    tempCantina = NAN;
    humCantina  = NAN;
  }

  if (bmpPresent) {
    float p = (bmp.readPressure() / 100.0f) + PRESS_OFFSET;
    if (!isnan(p) && p > 300.0f && p < 1200.0f) {
      pressCantina = p;
    } else {
      pressCantina = NAN;
    }
  } else {
    pressCantina = NAN;
  }

  addHistory(tempCantina, humCantina, pressCantina);

  Serial.print("Cellar temperature=");
  Serial.print(tempCantina, 1);
  Serial.print(" C  Cellar humidity=");
  Serial.print(humCantina, 1);
  Serial.print(" %  Cellar pressure=");
  Serial.print(pressCantina, 1);
  Serial.println(" hPa");
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleApiData() {
  String json;
  json.reserve(7000);

  bool ahtOk = !isnan(tempCantina) && !isnan(humCantina);
  bool bmpOk = !isnan(pressCantina);

  json += "{";
  json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
  json += "\"ip\":\"" + ipToString(WiFi.localIP()) + "\",";
  json += "\"uptime_s\":" + String(millis() / 1000UL) + ",";
  json += "\"aht_ok\":";
  json += (ahtOk ? "true" : "false");
  json += ",";
  json += "\"bmp_ok\":";
  json += (bmpOk ? "true" : "false");
  json += ",";
  json += "\"temperature_cantina\":" + jsonFloat(tempCantina, 1) + ",";
  json += "\"humidity_cantina\":" + jsonFloat(humCantina, 1) + ",";
  json += "\"pressure_cantina\":" + jsonFloat(pressCantina, 1) + ",";
  json += "\"hist_count\":" + String(histCount) + ",";
  json += "\"hist\":{";
  json += "\"temperature_cantina\":" + historyArrayToJson(histTemp, 1) + ",";
  json += "\"humidity_cantina\":" + historyArrayToJson(histHum, 1) + ",";
  json += "\"pressure_cantina\":" + historyArrayToJson(histPress, 1);
  json += "}";
  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

void handleApiHa() {
  String json;
  json.reserve(300);

  json += "{";
  json += "\"device\":\"" + String(DEVICE_NAME) + "\",";
  json += "\"temperature\":" + jsonFloat(tempCantina, 1) + ",";
  json += "\"humidity\":" + jsonFloat(humCantina, 1) + ",";
  json += "\"pressure\":" + jsonFloat(pressCantina, 1) + ",";
  json += "\"uptime_s\":" + String(millis() / 1000UL);
  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "404");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  for (uint16_t i = 0; i < HISTORY_LEN; i++) {
    histTemp[i]  = NAN;
    histHum[i]   = NAN;
    histPress[i] = NAN;
  }

  initSensors();
  connectWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/ha", HTTP_GET, handleApiHa);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Web server started.");
  Serial.print("Open: http://");
  Serial.print(ipToString(local_IP));
  Serial.println("/");

  readSensors();
  lastSensorRead = millis();
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    readSensors();
  }
}

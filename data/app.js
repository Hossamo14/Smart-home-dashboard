
console.log("APP.JS LOADED");



// =========================
// A) Single LED (Brightness)
// =========================
const sw = document.getElementById("led");
const brightness = document.getElementById("brightness");

function syncBrightnessUI() {
  brightness.disabled = !sw.checked;
}

sw.onchange = () => {
  fetch("/led?state=" + (sw.checked ? 1 : 0));
  syncBrightnessUI();
};

brightness.oninput = () => {
  fetch("/brightness?value=" + brightness.value);
};

syncBrightnessUI();


// =========================
// B) RGB LED (Color Temp)
// =========================
const swRGB = document.getElementById("ledSwitch");
const ct = document.getElementById("ct");

function syncCTUI() {
  ct.disabled = !swRGB.checked;                 // makes it not clickable
  ct.classList.toggle("faded", !swRGB.checked); // makes it look faded
}

swRGB.onchange = () => {
  fetch("/switch?state=" + (swRGB.checked ? 1 : 0));
  syncCTUI();

  // if turning ON, apply current slider value immediately
  if (swRGB.checked) fetch("/ct?value=" + ct.value);
};

ct.oninput = () => {
  fetch("/ct?value=" + ct.value);
};

// initial on page load
syncCTUI();


const alarmSound = document.getElementById("alarmSound");

let audioArmed = false;
let alarmActive = false; // 🔑 الحالة الفعلية للصوت

document.addEventListener("click", async () => {
  try {
    await alarmSound.play();
    alarmSound.pause();
    alarmSound.currentTime = 0;
    audioArmed = true;
    console.log("Audio unlocked");
  } catch {}
}, { once: true });

async function updateGas() {
  try {
    const r = await fetch("/api/gas", { cache: "no-store" });
    const d = await r.json();

    // ✅ حسم الحالة بشكل صريح
    const isAlarmOn = d.alarm === 1 || d.alarm === "1" || d.gas === 1;

    document.body.classList.toggle("alert-on", isAlarmOn);

    // 🔊 تشغيل / إيقاف بناءً على تغيّر الحالة فقط
    if (audioArmed && isAlarmOn && !alarmActive) {
      alarmSound.currentTime = 0;
      await alarmSound.play();
      alarmActive = true;
      console.log("ALARM ON 🔥");
    }

    if (!isAlarmOn && alarmActive) {
      alarmSound.pause();
      alarmSound.currentTime = 0;
      alarmActive = false;
      console.log("ALARM OFF ✅");
    }

  } catch (e) {
    // أمان
    alarmSound.pause();
    alarmSound.currentTime = 0;
    alarmActive = false;
    document.body.classList.remove("alert-on");
  }
}

updateGas();
setInterval(updateGas, 1000);



const cardTemp = document.getElementById("cardTempNum");
const cardHum  = document.getElementById("cardHumNum");

async function update() {
  const r = await fetch("/api/sensors");
  const d = await r.json();

  cardTemp.textContent = d.temp.toFixed(1);
  cardHum.textContent  = d.hum.toFixed(0);
}

update();
setInterval(update, 2000);


// ===============================
// DHT12 → LINE CHART (HTTP)
// ===============================

// Keep history (last N points)
const MAX_POINTS = 30;
const tempSeries = [];
const humSeries  = [];

// Chart elements
const tempPath = document.getElementById("tempPath");
const humPath  = document.getElementById("humPath");
const tempGlowPath = document.getElementById("tempGlowPath");
const humGlowPath  = document.getElementById("humGlowPath");
const tempLast = document.getElementById("tempLast");
const humLast  = document.getElementById("humLast");
const liveReadout = document.getElementById("liveReadout");

// SVG plot bounds (must match SVG)
const plot = {
  x0: 120,
  y0: 60,
  x1: 960,
  y1: 460
};

// Convert data array to SVG path
function makePath(series, minY, maxY) {
  const n = series.length;
  if (n < 2) return { d: "", last: { x: plot.x0, y: plot.y1 } };

  const dx = (plot.x1 - plot.x0) / (n - 1);

  const pts = series.map((v, i) => {
    const t = (v - minY) / (maxY - minY || 1);
    return {
      x: plot.x0 + dx * i,
      y: plot.y1 - t * (plot.y1 - plot.y0)
    };
  });

  const d = pts
    .map((p, i) => `${i === 0 ? "M" : "L"} ${p.x.toFixed(1)} ${p.y.toFixed(1)}`)
    .join(" ");

  return { d, last: pts[pts.length - 1] };
}

// Render chart
function renderChart() {
  if (tempSeries.length < 2) return;

  // Shared scale (clean digital-twin look)
  const all = tempSeries.concat(humSeries);
  let minY = Math.min(...all);
  let maxY = Math.max(...all);

  const pad = (maxY - minY) * 0.15 || 1;
  minY -= pad;
  maxY += pad;

  const t = makePath(tempSeries, minY, maxY);
  const h = makePath(humSeries,  minY, maxY);

  tempPath.setAttribute("d", t.d);
  humPath.setAttribute("d", h.d);
  tempGlowPath.setAttribute("d", t.d);
  humGlowPath.setAttribute("d", h.d);

  tempLast.setAttribute("cx", t.last.x);
  tempLast.setAttribute("cy", t.last.y);
  humLast.setAttribute("cx", h.last.x);
  humLast.setAttribute("cy", h.last.y);

  const tVal = tempSeries[tempSeries.length - 1];
  const hVal = humSeries[humSeries.length - 1];
  liveReadout.textContent = `${tVal.toFixed(1)} °C • ${hVal.toFixed(0)} %`;
}

// Fetch DHT data from ESP32
async function updateEnvChart() {
  try {
    const r = await fetch("/api/sensors");
    const d = await r.json();

    if (isNaN(d.temp) || isNaN(d.hum)) return;

    tempSeries.push(d.temp);
    humSeries.push(d.hum);

    if (tempSeries.length > MAX_POINTS) tempSeries.shift();
    if (humSeries.length  > MAX_POINTS) humSeries.shift();

    renderChart();
  } catch (e) {
    console.warn("Sensor fetch failed");
  }
}

// Initial fill (avoids empty chart)
for (let i = 0; i < 5; i++) {
  tempSeries.push(25);
  humSeries.push(50);
}
renderChart();

// Poll ESP32 every 2 seconds (matches your backend)
updateEnvChart();
setInterval(updateEnvChart, 2000);


// =========================
// RAIN EFFECT (HTTP)
// =========================
async function updateRain() {
  try {
    const r = await fetch("/api/rain", { cache: "no-store" });
    const d = await r.json();
    document.body.classList.toggle("rain-on", Number(d.rain) === 1);
  } catch (e) {
    document.body.classList.remove("rain-on");
  }
}

updateRain();
setInterval(updateRain, 1200);


// Run now + repeat
updateForecastAndEffects();
setInterval(updateForecastAndEffects, 2000);
  

// Clock
const el = document.getElementById("timeChip");
if (el) {
  const tick = () => {
    const d = new Date();
    const hh = String(d.getHours()).padStart(2, "0");
    const mm = String(d.getMinutes()).padStart(2, "0");
    el.textContent = `${hh}:${mm}`;
  };
  tick();
  setInterval(tick, 10000);
}


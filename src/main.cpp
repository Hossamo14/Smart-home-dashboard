#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <DHT.h>


WebServer server(80);

#define DHTPIN 16
#define DHTTYPE    DHT11 
DHT dht(DHTPIN, DHTTYPE);

//ESP32 uses hardware PWM (LEDC), not analogWrite.
const int LED_PIN_Br = 2;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RES = 8; // 8-bit (0–255)

int brightness = 0;
bool isOn = false;          // master switch
  // saved brightness (0–255)

  //RGB-led-Temprature//
  // RGB pins
  const int PIN_R = 14;
  const int PIN_G = 27;
  const int PIN_B = 26;
  // PWM Channels
  const int CH_R = 1;
  const int CH_G = 2;
  const int CH_B = 3;

  bool rgbOn = false; // master ON/OFF
  int lastV = 50;         // remember last slider position (0..100)


  // RAIN SENSOR //
  #define POWER_PIN 19
  #define DO_PIN 21

// Read rain sensor once (powers it briefly, reads DO, then powers off)
bool isRaining() {
  digitalWrite(POWER_PIN, HIGH);  // power ON
  delay(10);                      // stabilize
  int state = digitalRead(DO_PIN);
  digitalWrite(POWER_PIN, LOW);   // power OFF

  // Your original logic: HIGH = NOT detected, LOW = detected
  return (state == LOW);
}






  // Gas Sensor

  const int MQ2_PIN = 34;   // ADC pin (input only)
  const int GAS_THRESHOLD = 1880; 
  //Situation	Reading (example)
  // Fresh air	800–1200
  // Small gas	1300–1600
  // Strong gas	1800–3000
 // Linear Blend values
  // 0% → all color A
  // 100% → all color B
  // 50% → half A, half B

  // This line does that:
  // mix(a, b, t)
  // “Give me a value between a and b depending on t%”
  // mix(10, 20, 0)   → 10
  // mix(10, 20, 50)  → 15
  // mix(10, 20, 100) → 20

int mix(int a, int b, int t) {
  return a + (b - a) * t / 100;
}

void writeRGB(int r, int g, int b) {
  ledcWrite(CH_R, r);
  ledcWrite(CH_G, g);
  ledcWrite(CH_B, b);
}
  




// ---------- HANDLERS ----------

// GET /
void handleRoot() {
  File f = LittleFS.open("/index.html", "r");
  server.streamFile(f, "text/html");
  f.close();
}

// GET /styles.css
void handleCSS() {
  File f = LittleFS.open("/styles.css", "r");
  server.streamFile(f, "text/css");
  f.close();
}
// GET /app.js
void handleJS() {
  if (!LittleFS.exists("/app.js")) {
    server.send(404, "text/plain", "app.js not found");
    return;
  }

  File f = LittleFS.open("/app.js", "r");
  if (!f) {
    server.send(500, "text/plain", "Failed to open app.js");
    return;
  }

  server.streamFile(f, "application/javascript"); // ✅ correct type
  f.close();
}

void applyCT(int v) {
  int r, g, b;

  if (v <= 50) {
    int t = v * 2;
    r = mix(255, 255, t);
    g = mix(160, 255, t);
    b = mix( 60, 255, t);
  } else {
    int t = (v - 50) * 2;
    r = mix(255, 180, t);
    g = mix(255, 220, t);
    b = mix(255, 255, t);
  }

  writeRGB(r, g, b);
   Serial.print("CT slider v = ");
  Serial.println(v);

}
// /ct?value=0..100  (color temperature slider)
void handleCT() {
  int v = server.arg("value").toInt();
  v = constrain(v, 0, 100);
  lastV = v;

  if (!rgbOn) {                 // ✅ respect switch
    server.send(200, "text/plain", "off");
    return;
  }

  applyCT(v);
  server.send(200, "text/plain", "ok");
}


void handleRGBSwitch() {
 rgbOn = (server.arg("state") == "1");

  if (!rgbOn) {
    writeRGB(0, 0, 0);          // ✅ force OFF
  } else {
    applyCT(lastV);             // ✅ restore last color
  }

  server.send(200, "text/plain", "ok");
  
}


void handleGas(){
  int gas = analogRead(MQ2_PIN); // 0..4045
  bool alarm =gas >= GAS_THRESHOLD; // true if high

  String json = "{";
  json += "\"gas\":" + String(gas) + ",";
  json += "\"alarm\":" + String(alarm ? 1 : 0);
  json += "}";


  server.send(200, "application/json", json);
  Serial.println(gas);
}



// 404 handler
void handleNotFound() {
  server.send(404, "text/plain", "404 - File Not Found");
}


// void handleLed() {
//   String s = server.arg("state");
//   digitalWrite(LED_PIN, s == "1" ? HIGH : LOW);
//   server.send(200, "text/plain", "ok");
// }


void applyLed() {
  ledcWrite(PWM_CHANNEL, isOn ? brightness : 0);
}

// /switch?state=1 or /switch?state=0
void handleSwitch() {
  isOn = (server.arg("state") == "1");
  applyLed();
  server.send(200, "text/plain", "ok");
}

// /brightness?value=0..100
void handleBrightness() {
  int percent = server.arg("value").toInt();
  percent = constrain(percent, 0, 100);

  brightness = map(percent, 0, 100, 0, 255); // update saved level
  applyLed();                                // only lights if isOn=true

  server.send(200, "text/plain", "ok");
}


void handleSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
    server.send(503, "application/json", "{\"error\":\"DHT read failed\"}");
    return;
  }
  String json = "{";
  json += "\"temp\":" + String(t, 1) + ",";
  json += "\"hum\":"  + String(h, 1);
  json += "}";
  server.send(200, "application/json", json);
}

// GET /api/rain  -> {"rain":1} or {"rain":0}
void handleRainJson() {
  bool raining = isRaining();

  String json = "{";
  json += "\"rain\":" + String(raining ? 1 : 0);
  json += "}";

  server.send(200, "application/json", json);

  // Optional serial log (grammar fixed)
  Serial.println(raining ? "Rain detected." : "No rain detected.");
}


// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);

  dht.begin();

 

  // PWM setup
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN_Br, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  // RGB PWM
  ledcSetup(CH_R, PWM_FREQ, PWM_RES);
  ledcSetup(CH_G, PWM_FREQ, PWM_RES);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_R, CH_R);
  ledcAttachPin(PIN_G, CH_G);
  ledcAttachPin(PIN_B, CH_B);
  writeRGB(0, 0, 0);

  // Gas sensor
  pinMode(MQ2_PIN, INPUT);

  // Rain sensor power board
  pinMode(POWER_PIN, OUTPUT);
  pinMode(DO_PIN, INPUT);
  digitalWrite(POWER_PIN, LOW);

  // ✅ Mount filesystem BEFORE routes that use it
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed!");
  }

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin("WE_E74000", "10083792");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected");
  Serial.println(WiFi.localIP());

  // ---------- ROUTES ----------
  server.on("/", HTTP_GET, handleRoot);
  server.on("/styles.css", HTTP_GET, handleCSS);
  server.on("/app.js", HTTP_GET, handleJS);

  server.on("/led", HTTP_GET, handleSwitch);
  server.on("/brightness", HTTP_GET, handleBrightness);

  server.on("/api/rain", HTTP_GET, handleRainJson);
  server.on("/api/gas", HTTP_GET, handleGas);
  server.on("/api/sensors", HTTP_GET, handleSensors);

  server.on("/ct", HTTP_GET, handleCT);
  server.on("/switch", HTTP_GET, handleRGBSwitch);

  // ✅ If you want, keep a specific route:
  server.on("/Alert-sound.mp3", HTTP_GET, []() {
    File f = LittleFS.open("/Alert-sound.mp3", "r");
    if (!f) { server.send(404, "text/plain", "Missing Alert_sound.mp3"); return; }
    server.streamFile(f, "audio/mpeg");
    f.close();
  });

  // ✅ Only ONE onNotFound (this one can serve any file automatically)
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Server started");
}
 


void loop() {
  server.handleClient();


  
 
    
    
  
}


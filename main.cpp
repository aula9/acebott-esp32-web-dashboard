#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP32_Servo.h>

// -------------------- Hardware Pins --------------------
#define LED_PIN     19
#define LIGHT_PIN   33
#define SERVO_PIN   13

Servo doorServo;

// -------------------- WiFi Settings --------------------
const char* ssid     = "4G-MIFI-DA25";
const char* password = "1234567890";

WiFiServer server(80);

// -------------------- HTML Page --------------------
String htmlPage = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>ESP32 Control Panel</title>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>
  body {
    font-family: Arial, sans-serif;
    text-align: center;
    margin: 0;
    background: linear-gradient(135deg,#1e3c72,#2a5298);
    color: #333;
  }
  h1 { color: #fff; margin-top: 35px; }
  .subtitle { color:#d0e2ff; margin-bottom:25px; }
  .container { max-width:420px; margin:auto; }
  .card {
    background:#fff; border-radius:14px;
    padding:22px; margin-bottom:22px;
    box-shadow:0 8px 20px rgba(0,0,0,0.15);
  }
  button {
    width:160px; padding:12px; margin:8px;
    font-size:16px; border:none; border-radius:8px;
    cursor:pointer; transition:0.15s;
  }
  button:hover { opacity:0.85; transform:scale(1.03); }
  .on  { background:#28a745; color:white; }
  .off { background:#dc3545; color:white; }
  .door{ background:#007bff; color:white; }
  .status-label { font-size:14px; color:#666; }
  .status-value { font-size:22px; font-weight:bold; }
  .light-bar {
    width:100%; height:10px; background:#eee;
    border-radius:5px; overflow:hidden; margin-top:8px;
  }
  .light-fill {
    height:100%; width:0%;
    background:linear-gradient(90deg,#ffc107,#ff5722);
    transition:width 0.3s;
  }
</style>
</head>

<body>

<h1>ESP32 Control Panel</h1>
<div class="subtitle">LED • Door • Light Sensor</div>

<div class="container">

  <div class="card">
    <h2>Controls</h2>
    <button class="on"  onclick="send('a')">LED ON</button>
    <button class="off" onclick="send('b')">LED OFF</button>
    <button class="door" onclick="send('c')">OPEN DOOR</button>
  </div>

  <div class="card">
    <h2>Light Sensor</h2>
    <div class="status-label">Raw Value</div>
    <div id="light" class="status-value">--</div>

    <div class="status-label">Brightness</div>
    <div class="light-bar">
      <div id="lightFill" class="light-fill"></div>
    </div>
  </div>

</div>

<script>
function send(cmd){
  fetch("/cmd?val=" + cmd);
}

setInterval(()=>{
  fetch("/light")
    .then(r => r.text())
    .then(t => {
      document.getElementById("light").innerHTML = t;

      let v = parseInt(t);
      if (isNaN(v)) v = 0;

      let percent = Math.max(0, Math.min(100, 100 - (v / 4095 * 100)));
      document.getElementById("lightFill").style.width = percent + "%";
    });
}, 1000);
</script>

</body>
</html>
)=====";


// -------------------- Helper Functions --------------------

// Handle LED and door commands
void handleCommand(const String& req) {
  if (req.indexOf("val=a") >= 0) {
    digitalWrite(LED_PIN, HIGH);
  }
  else if (req.indexOf("val=b") >= 0) {
    digitalWrite(LED_PIN, LOW);
  }
  else if (req.indexOf("val=c") >= 0) {
    openDoor();
  }
}

// Send light sensor value
void sendLightValue(WiFiClient& client) {
  int value = analogRead(LIGHT_PIN);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.print(value);
}

// Door animation
void openDoor() {
  for (int angle = 0; angle <= 160; angle += 5) {
    doorServo.write(angle);
    delay(25);
  }
  delay(1500);
  for (int angle = 160; angle >= 0; angle -= 5) {
    doorServo.write(angle);
    delay(25);
  }
}


// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LIGHT_PIN, INPUT);

  doorServo.attach(SERVO_PIN);
  doorServo.write(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.begin();
}


// -------------------- Main Loop --------------------
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  while (!client.available()) delay(1);

  String req = client.readStringUntil('\r');
  client.flush();

  if (req.indexOf("/cmd") >= 0) {
    handleCommand(req);
  }
  else if (req.indexOf("/light") >= 0) {
    sendLightValue(client);
    return;
  }

  // Serve main page
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.print(htmlPage);
}

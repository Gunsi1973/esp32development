#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "xxx";
const char* password = "xxx";

// Sensor parameters (default values)
int sensor_th1 = 130;         // Motion Detection Sensitivity
int sensor_th2 = 250;         // Presence Detection Sensitivity
int sensor_th_in = 300;       // Internal threshold (unknown purpose)
int sensor_output_mode = 0;   // Output Mode (0 = standard ASCII text output)
int sensor_tons = 30;         // Output Hold Time
int sensor_utons = 100;       // Output Release Time
int rawOutputEnabled = 1;     // 1 = enable raw output to Serial monitor, 0 = disable

#define RX_PIN 4
#define TX_PIN 5

HardwareSerial LD1115H_Serial(2);
WebServer server(80);

String sensorData = "Waiting for data...";
unsigned long last_movement_time = 0;
unsigned long clear_time = 2000;  // 2 seconds until "No movement detected" is set

// Forward declarations
String getWebPage();

// Function to interpret sensor mode based on the provided value
String interpretMode(int mode) {
  switch (mode) {
    case 1: return "Very light movement";
    case 3: return "Small movement";
    case 5: return "Stronger movement";
    case 6: return "Calm presence";
    case 7: return "Presence remains stable";
    case 8: return "Clear presence";
    case 9: return "Very strong presence";
    default: return "Unknown Mode";
  }
}

// Function to send sensor configuration commands over serial
void configureSensor() {
  String cmd;
  
  cmd = "th1=" + String(sensor_th1);
  LD1115H_Serial.println(cmd);
  delay(50);
  
  cmd = "th2=" + String(sensor_th2);
  LD1115H_Serial.println(cmd);
  delay(50);
  
  cmd = "th_in=" + String(sensor_th_in);
  LD1115H_Serial.println(cmd);
  delay(50);
  
  cmd = "output_mode=" + String(sensor_output_mode);
  LD1115H_Serial.println(cmd);
  delay(50);
  
  cmd = "tons=" + String(sensor_tons);
  LD1115H_Serial.println(cmd);
  delay(50);
  
  cmd = "utons=" + String(sensor_utons);
  LD1115H_Serial.println(cmd);
  delay(50);
  
  // Optionally, send "save" if your sensor supports saving parameters.
  // LD1115H_Serial.println("save");
  // delay(50);
}

void setup() {
  Serial.begin(115200);
  // Start serial communication with the sensor
  LD1115H_Serial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  LD1115H_Serial.setTimeout(50);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure sensor parameters on startup
  configureSensor();

  // Set up web server endpoints
  server.on("/", []() {
    server.send(200, "text/html", getWebPage());
  });
  
  server.on("/data", []() {
    server.send(200, "text/plain", sensorData);
  });
  
  // Endpoint for dynamic configuration (AJAX calls from the main page)
  server.on("/config", []() {
    if (server.hasArg("submit")) {
      // Update parameters from the GET request values
      if (server.hasArg("th1")) {
        sensor_th1 = server.arg("th1").toInt();
      }
      if (server.hasArg("th2")) {
        sensor_th2 = server.arg("th2").toInt();
      }
      if (server.hasArg("th_in")) {
        sensor_th_in = server.arg("th_in").toInt();
      }
      if (server.hasArg("tons")) {
        sensor_tons = server.arg("tons").toInt();
      }
      if (server.hasArg("utons")) {
        sensor_utons = server.arg("utons").toInt();
      }
      if (server.hasArg("output_mode")) {
        sensor_output_mode = server.arg("output_mode").toInt();
      }
      if (server.hasArg("rawOutput")) {
        rawOutputEnabled = server.arg("rawOutput").toInt();
      }
      
      // Re-configure sensor with the new parameters
      configureSensor();
      
      // Log updated parameters to the Serial monitor (if raw output is enabled)
      Serial.println("Sensor parameters updated:");
      Serial.print("th1: "); Serial.println(sensor_th1);
      Serial.print("th2: "); Serial.println(sensor_th2);
      Serial.print("th_in: "); Serial.println(sensor_th_in);
      Serial.print("output_mode: "); Serial.println(sensor_output_mode);
      Serial.print("tons: "); Serial.println(sensor_tons);
      Serial.print("utons: "); Serial.println(sensor_utons);
      Serial.print("Raw output: "); Serial.println(rawOutputEnabled ? "Enabled" : "Disabled");
      
      server.send(200, "text/html", "OK");
    } else {
      server.send(200, "text/html", "No config received");
    }
  });
  
  server.begin();
}

void loop() {
  // Read sensor data from serial and optionally print the raw data to the Serial monitor
  if (LD1115H_Serial.available()) {
    String rawData = LD1115H_Serial.readStringUntil('\n');
    rawData.trim();
    
    if (rawOutputEnabled) {
      Serial.println("Raw sensor output: " + rawData);
    }
    
    if (rawData.length() > 0) {
      int firstComma = rawData.indexOf(',');
      int spaceAfterMode = rawData.indexOf(' ', firstComma + 2);
      
      if (firstComma != -1 && spaceAfterMode != -1) {
        String status = rawData.substring(0, firstComma);
        String modeStr = rawData.substring(firstComma + 2, spaceAfterMode);
        String value = rawData.substring(spaceAfterMode + 1);
        int signalStrength = value.toInt();
        int mode = modeStr.toInt();

        String modeDescription = interpretMode(mode);
        String newData = "";
        
        if (status == "mov") {
          newData = "🔵 Movement detected! (Mode: " + modeDescription + " | Signal: " + String(signalStrength) + ")";
        } else if (status == "occ") {
          newData = "🟢 Presence detected! (Mode: " + modeDescription + " | Signal: " + String(signalStrength) + ")";
        }
        
        if (newData != "" && sensorData != newData) {
          sensorData = newData;
          last_movement_time = millis();
        }
      }
    }
  }

  // If no movement is detected within clear_time, update sensorData accordingly
  if (millis() - last_movement_time > clear_time) {
    if (sensorData != "🔻 No movement detected.") {
      sensorData = "🔻 No movement detected.";
    }
  }

  server.handleClient();
}

// Integrated main page: header with sensor data, left configuration panel, and right live graph.
String getWebPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Sensor Monitor & Configuration</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@1.2.1"></script>
  <style>
    html, body {
      height: 100%;
      margin: 0;
      font-family: Arial, sans-serif;
    }
    #header {
      height: 20%;
      background: #f0f0f0;
      text-align: center;
      padding: 10px;
      box-sizing: border-box;
    }
    #container {
      display: flex;
      height: 80%;
    }
    #controls {
      width: 20%;
      background: #fafafa;
      padding: 10px;
      box-sizing: border-box;
      overflow-y: auto;
    }
    #graph {
      flex-grow: 1;
      padding: 10px;
      box-sizing: border-box;
    }
    h1 { color: #007BFF; }
    h2 { font-size: 1.2em; }
    .control { margin: 10px 0; }
    label { display: block; margin-bottom: 5px; }
    input[type=range] { width: 100%; }
    select { width: 100%; padding: 5px; }
    #status { color: green; margin-top: 10px; text-align: center; }
  </style>
</head>
<body>
  <div id="header">
    <h1>ESP32 Sensor Monitor</h1>
    <p id="data">Waiting for data...</p>
  </div>
  <div id="container">
    <div id="controls">
      <h2>Configuration</h2>
      <div class="control">
        <label for="th1">Motion Sensitivity (th1): <span id="th1Val">)rawliteral";
  page += String(sensor_th1);
  page += R"rawliteral(</span></label>
        <input type="range" id="th1" name="th1" min="0" max="1000" value=")rawliteral";
  page += String(sensor_th1);
  page += R"rawliteral(" oninput="document.getElementById('th1Val').innerText = this.value; updateConfig();">
      </div>
      <div class="control">
        <label for="th2">Presence Sensitivity (th2): <span id="th2Val">)rawliteral";
  page += String(sensor_th2);
  page += R"rawliteral(</span></label>
        <input type="range" id="th2" name="th2" min="0" max="1000" value=")rawliteral";
  page += String(sensor_th2);
  page += R"rawliteral(" oninput="document.getElementById('th2Val').innerText = this.value; updateConfig();">
      </div>
      <div class="control">
        <label for="th_in">Internal Threshold (th_in): <span id="th_inVal">)rawliteral";
  page += String(sensor_th_in);
  page += R"rawliteral(</span></label>
        <input type="range" id="th_in" name="th_in" min="0" max="1000" value=")rawliteral";
  page += String(sensor_th_in);
  page += R"rawliteral(" oninput="document.getElementById('th_inVal').innerText = this.value; updateConfig();">
      </div>
      <div class="control">
        <label for="tons">Output Hold Time (tons): <span id="tonsVal">)rawliteral";
  page += String(sensor_tons);
  page += R"rawliteral(</span></label>
        <input type="range" id="tons" name="tons" min="0" max="1000" value=")rawliteral";
  page += String(sensor_tons);
  page += R"rawliteral(" oninput="document.getElementById('tonsVal').innerText = this.value; updateConfig();">
      </div>
      <div class="control">
        <label for="utons">Output Release Time (utons): <span id="utonsVal">)rawliteral";
  page += String(sensor_utons);
  page += R"rawliteral(</span></label>
        <input type="range" id="utons" name="utons" min="0" max="1000" value=")rawliteral";
  page += String(sensor_utons);
  page += R"rawliteral(" oninput="document.getElementById('utonsVal').innerText = this.value; updateConfig();">
      </div>
      <div class="control">
        <label for="output_mode">Output Mode:</label>
        <select id="output_mode" name="output_mode" onchange="updateConfig();">
          <option value="0" )rawliteral";
  if(sensor_output_mode == 0) page += "selected";
  page += R"rawliteral(>Standard ASCII (0)</option>
          <option value="1" )rawliteral";
  if(sensor_output_mode == 1) page += "selected";
  page += R"rawliteral(>Mode 1 (1)</option>
        </select>
      </div>
      <div class="control">
        <label for="rawOutput">Raw Output to Serial:</label>
        <select id="rawOutput" name="rawOutput" onchange="updateConfig();">
          <option value="1" )rawliteral";
  if(rawOutputEnabled == 1) page += "selected";
  page += R"rawliteral(>Enable</option>
          <option value="0" )rawliteral";
  if(rawOutputEnabled == 0) page += "selected";
  page += R"rawliteral(>Disable</option>
        </select>
      </div>
      <div id="status"></div>
    </div>
    <div id="graph">
      <canvas id="chart"></canvas>
    </div>
  </div>
  <script>
    // Update configuration via AJAX request to /config
    function updateConfig() {
      let th1 = document.getElementById('th1').value;
      let th2 = document.getElementById('th2').value;
      let th_in = document.getElementById('th_in').value;
      let tons = document.getElementById('tons').value;
      let utons = document.getElementById('utons').value;
      let output_mode = document.getElementById('output_mode').value;
      let rawOutput = document.getElementById('rawOutput').value;
      let url = `/config?submit=true&th1=${th1}&th2=${th2}&th_in=${th_in}&tons=${tons}&utons=${utons}&output_mode=${output_mode}&rawOutput=${rawOutput}`;
      fetch(url)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = "Configuration updated";
          setTimeout(() => { document.getElementById('status').innerHTML = ""; }, 2000);
        })
        .catch(err => console.error("Error updating config", err));
    }
    
    // Chart and live data handling
    let maxDataPoints = 75;  // Live display limit
    let maxStoragePoints = 21600;  // 6 hours of storage
    let dataPoints = [];
    let labels = [];
    let storedData = [];
    let chart; // Chart variable

    function getColor(signal) {
      if (signal < 5000) return 'green';
      if (signal < 15000) return 'yellow';
      return 'red';
    }

    function updateData() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          document.getElementById('data').innerText = data;
          let match = data.match(/Signal: (\d+)/);
          if (match) {
            let signal = parseInt(match[1]);
            let timestamp = new Date().toLocaleTimeString().split(" ")[0];
            while (dataPoints.length >= maxDataPoints) {
              dataPoints.shift();
              labels.shift();
            }
            dataPoints.push(signal);
            labels.push(timestamp);
            if (storedData.length >= maxStoragePoints) {
              storedData.shift();
            }
            storedData.push({ time: timestamp, value: signal });
            chart.data.labels = labels;
            chart.data.datasets[0].data = dataPoints;
            chart.data.datasets[0].borderColor = getColor(signal);
            chart.data.datasets[0].backgroundColor = getColor(signal) + '33';
            chart.update();
          }
        })
        .catch(error => console.error("Error fetching data:", error));
    }
    
    window.onload = function() {
      let ctx = document.getElementById('chart').getContext('2d');
      chart = new Chart(ctx, {
        type: 'line',
        data: {
          labels: [],
          datasets: [{
            label: 'Signal Strength',
            data: [],
            borderColor: 'blue',
            backgroundColor: 'rgba(0, 123, 255, 0.2)',
            fill: true,
            tension: 0.1
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            x: { display: true },
            y: { 
              beginAtZero: true,
              suggestedMax: function() { return Math.max(...dataPoints, 1000) + 1000; } 
            }
          },
          plugins: {
            zoom: {
              pan: {
                enabled: true,
                mode: 'xy'
              },
              zoom: {
                enabled: true,
                mode: 'xy',
                speed: 0.1,
                limits: {
                  x: { min: 10, max: 21600 },
                  y: { min: 0 }
                }
              }
            }
          }
        }
      });
      setInterval(updateData, 1000);
    };
  </script>
</body>
</html>
)rawliteral";
  return page;
}

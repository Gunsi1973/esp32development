#include <WiFi.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "xxx";
const char* password = "xxx";

#define RX_PIN 4
#define TX_PIN 5

HardwareSerial LD1115H_Serial(2);
WebServer server(80);

String sensorData = "Waiting for data...";
unsigned long last_movement_time = 0;
unsigned long clear_time = 2000;  // 2 seconds until "No movement detected" is set

// Sensor configuration parameters
const int sensor_th1 = 130;         // Motion Detection Sensitivity (Default is 120)
const int sensor_th2 = 250;         // Presence Detection Sensitivity
const int sensor_th_in = 300;       // Unknown - Possibly an internal threshold for signal processing
const int sensor_output_mode = 0;   // Output Mode - 0 means standard ASCII text output
const int sensor_tons = 30;         // Output Hold Time
const int sensor_utons = 100;       // Output Release Time

// Forward declaration of getWebPage()
String getWebPage();

// Function to interpret sensor mode
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

// Function to configure sensor parameters
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
}

void setup() {
  Serial.begin(115200);
  // Begin serial communication with sensor
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

  // Set up web server routes
  server.on("/", []() {
    server.send(200, "text/html", getWebPage());
  });
  server.on("/data", []() {
    server.send(200, "text/plain", sensorData);
  });
  server.begin();
}

void loop() {
  if (LD1115H_Serial.available()) {
    String rawData = LD1115H_Serial.readStringUntil('\n');
    rawData.trim();
    
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

  // If no movement for the clear_time duration, set "No movement detected"
  if (millis() - last_movement_time > clear_time) {
    if (sensorData != "🔻 No movement detected.") {
      sensorData = "🔻 No movement detected.";
    }
  }

  server.handleClient();
}

// HTML + JavaScript for live updates and chart display
String getWebPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Sensor Monitor</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@1.2.1"></script>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; padding: 20px; }
    h1 { color: #007BFF; }
    #data { font-size: 1.5em; color: #333; margin-top: 20px; }
    #chart-container { width: 100%; height: 60vh; }
    canvas { width: 100% !important; height: 100% !important; }
    #controls { margin-top: 10px; }
    input[type="number"] { width: 60px; padding: 5px; text-align: center; }
  </style>
</head>
<body>
  <h1>ESP32 Sensor Monitor</h1>
  <p>Live sensor data:</p>
  <p id="data">Waiting for data...</p>

  <div id="controls">
    <label for="dataPoints">Data Points:</label>
    <input type="number" id="dataPoints" min="10" max="200" value="75">
    <button id="downloadCsv">Download CSV</button>
  </div>

  <div id="chart-container">
    <canvas id="chart"></canvas>
  </div>

  <script>
    let maxDataPoints = 75;  // Live display limit
    let maxStoragePoints = 21600;  // 6 hours of storage
    let dataPoints = [];
    let labels = [];
    let storedData = [];
    let chart; // Declare the chart variable

    document.getElementById("dataPoints").addEventListener("change", function() {
      maxDataPoints = parseInt(this.value);
    });

    document.getElementById('downloadCsv').addEventListener('click', function() {
      let csvContent = "data:text/csv;charset=utf-8,Time,Signal Strength\n";
      storedData.forEach(entry => {
        csvContent += `${entry.time},${entry.value}\n`;
      });
      const encodedUri = encodeURI(csvContent);
      const link = document.createElement("a");
      link.setAttribute("href", encodedUri);
      link.setAttribute("download", "sensor_data_6h.csv");
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    });

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

            // Manage live data (for graph display)
            while (dataPoints.length >= maxDataPoints) {
              dataPoints.shift();
              labels.shift();
            }
            dataPoints.push(signal);
            labels.push(timestamp);

            // Store data for CSV (only last 6 hours)
            if (storedData.length >= maxStoragePoints) {
              storedData.shift();  // Remove oldest entry
            }
            storedData.push({ time: timestamp, value: signal });

            // Update chart color
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
}

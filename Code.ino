#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <math.h>

// WiFi credentials
const char* ssid = "wifi_name";
const char* password = "wifi_password";

// Create a web server on port 80
ESP8266WebServer server(80);
const int ADDITIONAL_LED_PIN = 4; // GPIO4 (D2)
// HTML content to request GPS location
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>GPS Geofencing Interface</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      body {
        background-color: #282c34;
        color: #f5f5f5;
        font-family: Arial, sans-serif;
        text-align: center;
        padding: 20px;
      }

      button {
        padding: 10px 20px;
        margin: 10px;
        border: none;
        border-radius: 5px;
        background-color: #4CAF50;
        color: white;
        font-size: 16px;
        cursor: pointer;
      }

      button:hover {
        background-color: #45a049;
      }

      button:active {
        background-color: #3e8e41;
      }

      #status {
        margin-top: 20px;
        font-size: 16px;
        color: #f5f5f5;
      }
    </style>
   <script>
  let watchId;
  let isMusicAutoplayEnabled = false; // Tracks if autoplay state is enabled
  let beepAudio;

  // Function to set the MP3 source dynamically
  function setAudioSource(url) {
    if (beepAudio) {
      beepAudio.pause(); // Pause any existing audio
      beepAudio.src = url; // Update the source
      beepAudio.load(); // Reload the audio
    } else {
      beepAudio = new Audio(url); // Initialize new Audio object
    }
  }

  // Enable autoplay state and start continuous location updates
  function startContinuousLocation() {
    const statusElement = document.getElementById("status");
    if (navigator.geolocation) {
      statusElement.innerHTML = "Starting continuous location updates. Please allow location access.";
      watchId = navigator.geolocation.watchPosition(
        showPosition,
        showError,
        { enableHighAccuracy: true, maximumAge: 0, timeout: 5000 }
      );
      isMusicAutoplayEnabled = true; // Enable autoplay state
      console.log("Music autoplay state enabled.");
    } else {
      statusElement.innerHTML = "Geolocation not supported by your browser.";
    }
  }

  // Stop continuous location updates and disable autoplay state
  function stopContinuousLocation() {
    if (watchId !== undefined) {
      navigator.geolocation.clearWatch(watchId);
      watchId = undefined;
      isMusicAutoplayEnabled = false; // Disable autoplay state
      if (beepAudio) {
        beepAudio.pause(); // Ensure beep sound stops
      }
      document.getElementById("status").innerHTML = "Location updates stopped.";
      console.log("Music autoplay state disabled.");
    }
  }

  // Handle successful location acquisition
  function showPosition(position) {
    const lat = position.coords.latitude;
    const lon = position.coords.longitude;

    // Display location
    document.getElementById("lat").innerHTML = `Latitude: ${lat}`;
    document.getElementById("long").innerHTML = `Longitude: ${lon}`;
    document.getElementById("distance").innerHTML = "Distance from center: Calculating...";
    document.getElementById("circleStatus").innerHTML = "Circle Geofence: Checking...";
    document.getElementById("polygonStatus").innerHTML = "Square Geofence: Checking...";

    // Send location to server
    fetch(`/location?lat=${lat}&lon=${lon}`)
      .then(response => response.text())
      .then(data => {
        console.log("Location sent:", data);
        const lines = data.split('\n');
        lines.forEach(line => {
          if (line.startsWith("Distance from Circle Center:")) {
            document.getElementById("distance").innerHTML = line;
          }
          if (line.startsWith("Circle Geofence:")) {
            document.getElementById("circleStatus").innerHTML = line;
          }
          if (line.startsWith("Square Geofence:")) {
            document.getElementById("polygonStatus").innerHTML = line;
          }
        });

        // Play or pause beeps based on geofence status
        const circleStatus = document.getElementById("circleStatus").innerHTML;
        const squareStatus = document.getElementById("polygonStatus").innerHTML;
        if (isMusicAutoplayEnabled &&
            (circleStatus.includes("Outside") || squareStatus.includes("Inside"))) {
          beepAudio.loop = true; // Set beep sound to loop
          beepAudio.play().catch(error => console.error("Error playing beep:", error));
        } else {
          beepAudio.pause();
        }
      })
      .catch(error => {
        console.error("Error:", error);
        document.getElementById("status").innerHTML = "Error sending location data.";
      });
  }

  // Handle errors
  function showError(error) {
    let message = "";
    switch (error.code) {
      case error.PERMISSION_DENIED:
        message = "Permission denied. Please allow location access.";
        break;
      case error.POSITION_UNAVAILABLE:
        message = "Location information unavailable.";
        break;
      case error.TIMEOUT:
        message = "Location request timed out.";
        break;
      default:
        message = "An unknown error occurred.";
    }
    document.getElementById("status").innerHTML = message;
  }

  // Example: Set your MP3 URL here
  setAudioSource('https://www.pagalworld.com.sb/files/download/type/320/id/71764/1.mp3?1');
</script>

  </head>
  <body>
    <h1>GPS Geofencing Interface</h1>
    <div id="lat">Latitude: --</div>
    <div id="long">Longitude: --</div>
    <div id="distance">Distance from center: --</div>
    <div id="circleStatus">Circle Geofence: --</div>
    <div id="polygonStatus">Square Geofence: --</div>
    <button onclick="startContinuousLocation()">Start Location Updates</button>
    <button onclick="stopContinuousLocation()">Stop Location Updates</button>
    <p id="status">Status: Waiting for action...</p>
  </body>
</html>
)rawliteral";


// HTML for information page
const char* infoPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Real-Time Geofence Info</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: 'Arial', sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f4f9ff;
            color: #333;
        }
        header {
            background: linear-gradient(90deg, #3f51b5, #1a237e);
            color: white;
            padding: 1.5rem;
            text-align: center;
            font-size: 2rem;
            font-weight: bold;
            text-shadow: 0 2px 4px rgba(0, 0, 0, 0.4);
            position: relative;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.2);
        }
        .container {
            max-width: 90%;
            margin: 20px auto;
            padding: 15px;
            background: #ffffff;
            border-radius: 12px;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
            animation: fadeIn 1.5s ease-in-out;
            text-align: center;
        }
        p {
            font-size: 1rem;
            margin-bottom: 15px;
            line-height: 1.6;
            position: relative;
            transition: color 0.3s;
        }
        p span {
            font-weight: bold;
            color: #3f51b5;
            font-size: 1.2rem;
            animation: pulse 2s infinite;
        }
        p:hover {
            color: #1a237e;
        }
        .container:before {
            content: '';
            position: absolute;
            top: -10px;
            left: 50%;
            transform: translateX(-50%);
            width: 50%;
            height: 5px;
            background: linear-gradient(90deg, #3f51b5, #1a237e, #3f51b5);
            border-radius: 5px;
            animation: slideIn 3s infinite;
        }
        footer {
            text-align: center;
            margin: 20px 0;
            font-size: 0.9rem;
            color: #555;
        }
        footer a {
            color: #3f51b5;
            text-decoration: none;
        }
        footer a:hover {
            text-decoration: underline;
        }
        @keyframes fadeIn {
            from {
                opacity: 0;
                transform: translateY(20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        @keyframes pulse {
            0%, 100% {
                color: #3f51b5;
            }
            50% {
                color: #5c6bc0;
            }
        }
        @keyframes slideIn {
            0% {
                transform: translateX(-100%);
            }
            50% {
                transform: translateX(0);
            }
            100% {
                transform: translateX(100%);
            }
        }
        /* Responsive adjustments */
        @media (max-width: 768px) {
            header {
                font-size: 1.5rem;
                padding: 1rem;
            }
            .container {
                margin: 15px auto;
                padding: 10px;
                max-width: 95%;
            }
            p {
                font-size: 0.9rem;
            }
            p span {
                font-size: 1rem;
            }
        }
        @media (max-width: 480px) {
            header {
                font-size: 1.2rem;
                padding: 0.8rem;
            }
            p {
                font-size: 0.8rem;
            }
            p span {
                font-size: 0.9rem;
            }
        }
    </style>
</head>
<body>
    <header>
        Real-Time Geofence Info
    </header>
    <div class="container">
        <p>Latitude: <span id="lat">--</span></p>
        <p>Longitude: <span id="long">--</span></p>
        <p>Distance from center: <span id="distance">--</span></p>
        <p>Geofence Status: <span id="geofence">--</span></p>
    </div>
    <script>
        async function fetchInfo() {
            try {
                const response = await fetch('/geofenceInfo');
                const data = await response.json();

                document.getElementById('lat').textContent = data.latitude;
                document.getElementById('long').textContent = data.longitude;
                document.getElementById('distance').textContent = `${data.distance} meters`;
                document.getElementById('geofence').textContent = data.status;
            } catch (error) {
                console.error("Error fetching geofence info:", error);
            }
        }

        setInterval(fetchInfo, 1000); // Update info every 1 seconds
    </script>
</body>
</html>

)rawliteral";

// Constants for Earth radius
const double EARTH_RADIUS_M = 6371000.0; // Radius of the Earth in meters

// Circle geofence parameters
const double circleCenterLat = 30.96697840;
const double circleCenterLon = 76.46586580;
const double circleRadius = 5.0; // Radius in meters

// Variables to store geofence data
double currentLat = 0.0;
double currentLon = 0.0;
double currentDistance = 0.0;
String geofenceStatus = "Unknown";

// Square geofence parameters (vertices on the circle)
const double squareFence[][2] = {
  {30.96683218, 76.46623716},
  {30.96676862, 76.46623716},
  {30.96676862, 76.46616224},
  {30.96683218, 76.46616224}
};
const int squareFenceSize = sizeof(squareFence) / sizeof(squareFence[0]);

// Function to convert degrees to radians
double toRadians(double degrees) {
  return degrees * M_PI / 180.0;
}

// Function to calculate distance between two coordinates in meters using the Haversine formula
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  double dLat = toRadians(lat2 - lat1);
  double dLon = toRadians(lon2 - lon1);
  double rLat1 = toRadians(lat1);
  double rLat2 = toRadians(lat2);

  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(rLat1) * cos(rLat2) *
             sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return EARTH_RADIUS_M * c; // Distance in meters
}

// Geofencing logic: Check if a point is inside the polygon using the Ray Casting algorithm
bool isPointInPolygon(double lat, double lon) {
  bool inside = false;
  for (int i = 0, j = squareFenceSize - 1; i < squareFenceSize; j = i++) {
    double xi = squareFence[i][0], yi = squareFence[i][1];
    double xj = squareFence[j][0], yj = squareFence[j][1];

    bool intersect = ((yi > lon) != (yj > lon)) &&
                     (lat < (xj - xi) * (lon - yi) / (yj - yi) + xi);
    if (intersect)
      inside = !inside;
  }
  return inside;
}

// Function to check if a point is inside the circle geofence
bool isPointInCircle(double lat, double lon) {
  double distance = calculateDistance(lat, lon, circleCenterLat, circleCenterLon);
  return distance <= circleRadius; // Compare distance in meters
}

// Handle location data
void handleLocation() {
  if (server.hasArg("lat") && server.hasArg("lon")) {
    // Parse latitude and longitude as double to preserve precision
    double latitude = server.arg("lat").toDouble();
    double longitude = server.arg("lon").toDouble();
    currentLat = latitude;
    currentLon = longitude;
    // Calculate distance from the center
    double distanceFromCenter = calculateDistance(latitude, longitude, circleCenterLat, circleCenterLon);
    currentDistance = distanceFromCenter;
    // Check if the point is inside the geofences
    bool inCircle = isPointInCircle(latitude, longitude);
    bool inSquare = isPointInPolygon(latitude, longitude);

    // Control the LED based on geofence status
    if (inCircle || inSquare) {
        geofenceStatus = "Inside";
      digitalWrite(LED_BUILTIN, HIGH); // Turn off LED (active LOW)
      digitalWrite(ADDITIONAL_LED_PIN, HIGH); // Turn on additional LED
    } else {
      digitalWrite(LED_BUILTIN, LOW); 
      geofenceStatus = "Outside"; // Turn on LED (active LOW)
      digitalWrite(ADDITIONAL_LED_PIN, LOW); // Turn off additional LED
    }

    // Prepare response
    String response = "Latitude: " + String(latitude, 8) + "\n";
    response += "Longitude: " + String(longitude, 8) + "\n";
    response += "Distance from Circle Center: " + String(distanceFromCenter, 2) + " meters\n";
    response += "Circle Geofence: " + String(inCircle ? "Inside" : "Outside") + "\n";
    response += "Square Geofence: " + String(inSquare ? "Inside" : "Outside");

    // Send response
    server.send(200, "text/plain", response);

    // Print to Serial for debugging
    Serial.println(response);
  } else {
    server.send(400, "text/plain", "Invalid Request: Missing lat or lon parameter.");
  }
}


// Handle geofence info for the second page
void handleGeofenceInfo() {
  String json = "{";
  json += "\"latitude\":" + String(currentLat, 8) + ",";
  json += "\"longitude\":" + String(currentLon, 8) + ",";
  json += "\"distance\":" + String(currentDistance, 2) + ",";
  json += "\"status\":\"" + geofenceStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(100); // Give time for Serial to initialize

  // Configure LED_BUILTIN pin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Ensure LED is off initially
  pinMode(ADDITIONAL_LED_PIN, OUTPUT);
  digitalWrite(ADDITIONAL_LED_PIN, LOW); // Ensure additional LED is off initially

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Serve the HTML page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  // Handle location data
  server.on("/location", HTTP_GET, handleLocation);
  server.on("/geofenceInfo", HTTP_GET, handleGeofenceInfo);
  server.on("/info", HTTP_GET, []() {
    server.send(200, "text/html", infoPage);
  });


  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "AirFiber-aiRoh8";
const char* password = "@Alonegameryg184";

AsyncWebServer server(80);

// Pin Assignments
const int relayBulb = 26;
const int relayFan = 27;
const int ldrPin = 34;
const int trigPin = 12;
const int echoPin = 13;

bool autoMode = true;
int tempThreshold = 27;

void setup() {
  Serial.begin(115200);

  // Initialize relays (IMPORTANT: set HIGH before pinMode to prevent startup flicker)
  digitalWrite(relayBulb, HIGH);
  pinMode(relayBulb, OUTPUT);

  digitalWrite(relayFan, HIGH);
  pinMode(relayFan, OUTPUT);

  // Initialize other pins
  pinMode(ldrPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  // Route: HTML Page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    int ldrVal = analogRead(ldrPin);
    float temp = dht.readTemperature();

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<title>ESP32 Home Automation</title>";
    html += "<style>body{font-family:sans-serif;}</style></head><body>";
    html += "<h2>Home Automation Control</h2>";

    html += "<p>Mode: ";
    html += (autoMode) ? "Auto" : "Manual";
    html += "</p>";
    html += "<form action='/toggle' method='get'>";
    html += "<button type='submit'>Toggle Mode</button></form>";

    html += "<p>Temperature: " + String(temp) + " °C</p>";
    html += "<p>LDR Value: " + String(ldrVal) + "</p>";

    html += "<form action='/threshold' method='get'>";
    html += "Set Temp Threshold: <input type='number' name='value' value='" + String(tempThreshold) + "'>";
    html += "<input type='submit' value='Set'></form>";

    if (!autoMode) {
      html += "<form action='/bulbOn'><button>Turn Bulb ON</button></form>";
      html += "<form action='/bulbOff'><button>Turn Bulb OFF</button></form>";
      html += "<form action='/fanOn'><button>Turn Fan ON</button></form>";
      html += "<form action='/fanOff'><button>Turn Fan OFF</button></form>";
    }

    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Toggle auto/manual mode
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    autoMode = !autoMode;
    request->redirect("/");
  });

  // Update temp threshold
  server.on("/threshold", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value")) {
      tempThreshold = request->getParam("value")->value().toInt();
    }
    request->redirect("/");
  });

  // Manual controls
  server.on("/bulbOn", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayBulb, LOW);
    request->redirect("/");
  });
  server.on("/bulbOff", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayBulb, HIGH);
    request->redirect("/");
  });
  server.on("/fanOn", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayFan, LOW);
    request->redirect("/");
  });
  server.on("/fanOff", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayFan, HIGH);
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  if (autoMode) {
    // LDR and motion for bulb
    int ldrVal = analogRead(ldrPin);
    long duration, distance;

    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;

    if (distance < 100 && ldrVal < 1000) {
      digitalWrite(relayBulb, LOW); // turn bulb ON
    } else {
      digitalWrite(relayBulb, HIGH); // turn bulb OFF
    }

    // Temperature for fan
    float temp = dht.readTemperature();
    if (temp > tempThreshold) {
      digitalWrite(relayFan, LOW); // fan ON
    } else {
      digitalWrite(relayFan, HIGH); // fan OFF
    }
  }

  delay(500);
}
#include <WiFi.h>
#include <HTTPClient.h>

#define RELAY_PIN 10 // GPIO10

const int healthCheckDelaySeconds = 60000;
const int restartPeriodSeconds = 30000;

const char* ssid = "TP-LINK_A2101E"; // replace with actual ssid
const char* password = ""; // replace with actual password
const char* testHost = "httpbin.org";

void setup() {
  Serial.begin(115200);
  delay(1000);
  initializeRelay();
  turnRelayOn();
  connectToWiFiNetwork();
}

void loop() {
  delay(healthCheckDelaySeconds);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    reconnectWiFi();
  }

  Serial.println("Checking internet connectivity health...");

  if (pingServer(testHost)) {
    Serial.println("Internet connection is OK!");
    return;
  }

  Serial.println("Internet connection failed! Restaring router...");
  turnRelayOff();
  delay(restartPeriodSeconds);
  turnRelayOn();
}

void reconnectWiFi() {
  WiFi.disconnect();
  delay(1000);
  WiFi.reconnect();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Reconnected to WiFi!");
}

void connectToWiFiNetwork() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initializeRelay() {
  pinMode(RELAY_PIN, OUTPUT);
}

void turnRelayOn()  {
  digitalWrite(RELAY_PIN, LOW);
}

void turnRelayOff()  {
  digitalWrite(RELAY_PIN, HIGH);
}

bool pingServer(const char* host) {
  HTTPClient http;

  Serial.print("Pinging server: ");
  Serial.println(host);

  http.begin(String("http://") + host + "/get");
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    http.end();
    return true;
  }

  Serial.print("HTTP Error: ");
  Serial.println(http.errorToString(httpResponseCode).c_str());
  http.end();
  return false;
}

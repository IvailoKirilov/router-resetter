#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>
#include <time.h>

#define RELAY_PIN 9

// Email
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587
#define AUTHOR_EMAIL "" // actual sender email
#define AUTHOR_PASSWORD "" // actual email password
#define RECIPIENT_EMAIL "" // actual recipient email

// Delays
const int healthCheckDelayMilliseconds = 3 * 1000;
const int restartPeriodMilliseconds = 15 * 1000;
const int bootingPeriodMilliseconds = 180 * 1000;

// WiFi
const char* ssid = ""; // actual ssid
const char* password = ""; // actual wifi password
const char* testHost = "httpbin.org";

// Restart
const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSeconds = 2 * 3600;
const int daylightOffsetSeconds = 3600;
time_t firstRestartTimestamp;
bool restartingRouter = false;

SMTPSession smtp;

void setup() {
  Serial.begin(115200);
  delay(1000);
  initializeRelay();
  turnRelayOn();
  connectToWiFiNetwork();
  configTime(gmtOffsetSeconds, daylightOffsetSeconds, ntpServer);
}

void loop() {
  delay(healthCheckDelayMilliseconds);
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    reconnectWiFi();
  }

  Serial.println("Checking internet connectivity health...");

  if (pingServer(testHost)) {
    if (restartingRouter) {
      restartingRouter = false;
      sendEmail();
    }

    Serial.println("Internet connection is OK!");
    return;
  }

  if (!restartingRouter) {
    restartingRouter = true;
    time(&firstRestartTimestamp);
  }

  Serial.println("Internet connection failed! Restaring router...");
  turnRelayOff();
  delay(restartPeriodMilliseconds);
  turnRelayOn();
  delay(bootingPeriodMilliseconds);
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

void sendEmail() {
  Session_Config config;

  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  SMTP_Message message;

  message.sender.name = "Lyutibrod SPS";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Router Restart Alert");
  message.addRecipient(F("Nikolay Kirilov"), RECIPIENT_EMAIL);

  String firstRestartTimestampAsText = formatTimestamp(firstRestartTimestamp);
  String htmlMsg = "<h1>Router has lost internet connection at '" + firstRestartTimestampAsText + "'<h1/>";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

String formatTimestamp(time_t timestamp) {
  struct tm* timeinfo = localtime(&timestamp);
  if (timeinfo == nullptr) {
    return "Invalid Time";
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

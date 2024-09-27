#include <WiFi.h>
#include <PubSubClient.h>
#include "TridentTD_LineNotify.h"
#include "esp_camera.h"

#define SSID "PoohRit"
#define PASSWORD "p009rit99"
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USER "" 
#define MQTT_PASSWORD ""
#define LINE_TOKEN "5tT6nLJL1n3hzzSq3SNyPWpulX1KEdFbNiY8fjecXct"

int alertType = 0;  // 0: no alert, 1: theft alert, 2: fire alarm

// Camera pin definition for ESP32-CAM
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Initialize WiFi
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("WiFi connected.");
  Serial.print("ESP Board MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize MQTT
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  
  reconnectMQTT(); // เชื่อมต่อ MQTT
  setupCamera(); // ฟังก์ชันการตั้งค่ากล้อง

  // Set Line Notify token
  LINE.setToken(LINE_TOKEN);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }
  
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

void reconnectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.begin(SSID, PASSWORD);
    delay(5000);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("clientId-C9njBLfa8f", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe("esp32cam/mqtt", 1); // Subscribe to topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String command;
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }
  command.trim();  // Remove any leading/trailing whitespace

  if (command == "THEFT_ALERT") {
    alertType = 1;
    captureAndSendImage(); // Capture and send image immediately
  } else if (command == "FIRE_ALARM") {
    alertType = 2;
    captureAndSendImage(); // Capture and send image immediately
  } else if (command == "TAKE_A_PICTURE") {
    alertType = 3;
    captureAndSendImage(); // Capture and send image immediately
  } else {
    alertType = 0;  // Reset to no alert for unknown commands
  }
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Frame size
  config.frame_size = FRAMESIZE_SVGA; // Try reducing frame size if needed
  config.jpeg_quality = 12; // Adjust JPEG quality for faster transmission
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Access the sensor and flip the image vertically
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);  // 1 = flip vertically, 0 = no flip
    s->set_hmirror(s, 0);  // 1 = mirror horizontally, 0 = no mirror
  }
}

void captureAndSendImage() {
  // Clear frame buffer if any data is lingering
  esp_camera_fb_return(esp_camera_fb_get()); // Ensure the frame buffer is cleared

  camera_fb_t * fb = esp_camera_fb_get(); // Capture the image
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  String message;
  if (alertType == 1) {
    message = "Theft Alert: Captured Image";
  } else if (alertType == 2) {
    message = "Fire Alarm: Captured Image";
  } else if (alertType == 3) {
    message = "Take a picture: Captured Image";
  }

  Serial.println("Sending image to Line Notify...");

  // Send captured image to LINE
  bool success = LINE.notifyPicture(message.c_str(), fb->buf, fb->len);

  if (success) {
    Serial.println("Image sent successfully!");
  } else {
    Serial.println("Failed to send image.");
  }

  esp_camera_fb_return(fb); // Return the frame buffer
  delay(500); // Delay to prevent rapid firing and ensure delivery
}

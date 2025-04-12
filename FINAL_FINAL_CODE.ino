#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PZEM004Tv30.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi và server
const char* ssid = "Dyy";
const char* password = "11111111";
const char* serverName = "http://192.168.222.202:5000/data";
const char* serverURL = "http://192.168.222.202:5000/power-data";

// Sinric Pro
#define APP_KEY     "8e10eb5d-2f08-4317-908b-322a540c6239"
#define APP_SECRET  "07f2aa77-e000-4d17-aed1-c6af2b2b8e37-51b7fd9f-232a-4fdc-83a8-833818b01bf5"
#define DEVICE_ID_1 "67d13b264dee339ca79b6f80"
#define DEVICE_ID_2 "67d13b566066f22be0656db5"

// Relay
#define RELAY_1 33
#define RELAY_2 32

HardwareSerial pzemSerial(1);
PZEM004Tv30 pzem(pzemSerial, 16, 17);

// Xử lý bật/tắt đèn
bool onPowerState1(const String &deviceId, bool &state) {
  digitalWrite(RELAY_1, state ? HIGH : LOW);
  Serial.printf("Light 1 %s\n", state ? "ON" : "OFF");
  return true;
}
bool onPowerState2(const String &deviceId, bool &state) {
  digitalWrite(RELAY_2, state ? HIGH : LOW);
  Serial.printf("Light 2 %s\n", state ? "ON" : "OFF");
  return true;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to WiFi!");

  dht.begin();
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);

  // SinricPro setup
  SinricProSwitch &myLight1 = SinricPro[DEVICE_ID_1];
  myLight1.onPowerState(onPowerState1);
  SinricProSwitch &myLight2 = SinricPro[DEVICE_ID_2];
  myLight2.onPowerState(onPowerState2);
  SinricPro.begin(APP_KEY, APP_SECRET);

  // Tạo task gửi dữ liệu
  xTaskCreatePinnedToCore(
    dataTask,      // Hàm task
    "DataTask",    // Tên task
    5000,          // Stack size
    NULL,          // Tham số
    1,             // Ưu tiên
    NULL,          // Handle
    1              // Chạy trên core 1
  );
}

void loop() {
  SinricPro.handle(); // Luôn chạy mượt
}

// Task gửi dữ liệu định kỳ
void dataTask(void * parameter) {
  const unsigned long interval = 10000; // 10 giây
  unsigned long lastSend = 0;

  while (true) {
    unsigned long now = millis();
    if (now - lastSend >= interval) {
      lastSend = now;
      ensureWiFi();

      // Đọc cảm biến DHT
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();

      if (!isnan(temperature) && !isnan(humidity)) {
        sendHumidity(temperature, humidity);
      }

      // Đọc cảm biến PZEM
      float voltage = pzem.voltage();
      float current = pzem.current();
      float power = pzem.power();
      float energy = pzem.energy();
      float frequency = pzem.frequency();
      float pf = pzem.pf();

      sendToServer(voltage, current, power, energy, frequency, pf);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Nghỉ 100ms
  }
}

// Gửi nhiệt độ và độ ẩm
void sendHumidity(float temperature, float humidity) {
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["temperature"] = temperature;
  jsonDoc["humidity"] = humidity;

  String requestData;
  serializeJson(jsonDoc, requestData);

  int httpResponseCode = http.POST(requestData);
  if (httpResponseCode > 0) {
    Serial.println("Humidity sent: " + http.getString());
  } else {
    Serial.println("Humidity send error");
  }
  http.end();
}

// Gửi dữ liệu điện
void sendToServer(float voltage, float current, float power, float energy, float frequency, float pf) {
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> jsonDoc;
  jsonDoc["voltage"] = voltage;
  jsonDoc["current"] = current;
  jsonDoc["power"] = power;
  jsonDoc["energy"] = energy;
  jsonDoc["frequency"] = frequency;
  jsonDoc["power_factor"] = pf;

  String jsonStr;
  serializeJson(jsonDoc, jsonStr);
  Serial.println("Sending power data...");

  int httpResponseCode = http.POST(jsonStr);
  if (httpResponseCode > 0) {
    Serial.println("Power data sent: " + http.getString());
  } else {
    Serial.printf("HTTP error: %d\n", httpResponseCode);
  }

  http.end();
}

// Kiểm tra kết nối WiFi
void ensureWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nReconnected!");
  }
}

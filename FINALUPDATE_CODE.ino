#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <PZEM004Tv30.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// ================== CẤU HÌNH PHẦN CỨNG ==================
#define DHTPIN 4
#define DHTTYPE DHT11
#define RELAY1_PIN 33
#define RELAY2_PIN 32

// ================== SINRIC PRO ==================
#define APP_KEY     "8e10eb5d-2f08-4317-908b-322a540c6239"
#define APP_SECRET  "07f2aa77-e000-4d17-aed1-c6af2b2b8e37-51b7fd9f-232a-4fdc-83a8-833818b01bf5"
#define DEVICE_ID_1 "67d13b264dee339ca79b6f80"
#define DEVICE_ID_2 "67d13b566066f22be0656db5"

// ================== WIFI & SERVER ==================
const char* ssid = "Dyy";
const char* password = "11111111";
const char* serverHumidity = "http://192.168.222.202:5000/data";
const char* serverPower = "http://192.168.222.202:5000/power-data";

// ================== MQTT HIVE MQ CLOUD ==================
const char* mqtt_server = "e17c27e51dd64ab99f00a1ea2ca57bca.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "hivemq.webclient.1744615444575";
const char* mqtt_pass = "Dt8KjA7p9;>s6ByEo&C#";

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
HardwareSerial pzemSerial(1);
PZEM004Tv30 pzem(pzemSerial, 16, 17);

// ================== SINRIC CALLBACK ==================
bool onPowerState1(const String &deviceId, bool &state) {
  digitalWrite(RELAY1_PIN, state ? HIGH : LOW);
  Serial.printf("Light 1 %s\n", state ? "ON" : "OFF");
  return true;
}
bool onPowerState2(const String &deviceId, bool &state) {
  digitalWrite(RELAY2_PIN, state ? HIGH : LOW);
  Serial.printf("Light 2 %s\n", state ? "ON" : "OFF");
  return true;
}

// ================== SETUP WIFI ==================
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Kết nối WiFi thành công!");
  Serial.print("📶 IP: ");
  Serial.println(WiFi.localIP());
}

// ================== MQTT CALLBACK ==================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("📥 MQTT command: ");
  Serial.println(msg);

  if (msg == "LIGHT1_ON") digitalWrite(RELAY1_PIN, HIGH);
  else if (msg == "LIGHT1_OFF") digitalWrite(RELAY1_PIN, LOW);
  else if (msg == "LIGHT2_ON") digitalWrite(RELAY2_PIN, HIGH);
  else if (msg == "LIGHT2_OFF") digitalWrite(RELAY2_PIN, LOW);
}

// ================== MQTT RECONNECT ==================
void reconnect() {
  while (!client.connected()) {
    Serial.print("🔌 Kết nối lại MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("✅ MQTT connected!");
      client.subscribe("home/light");
    } else {
      Serial.print("❌ MQTT failed. Code: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// ================== GỬI DỮ LIỆU ==================
void sendHumidity(float temperature, float humidity) {
  HTTPClient http;
  http.begin(serverHumidity);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;

  String json;
  serializeJson(doc, json);
  int code = http.POST(json);

  if (code > 0)
    Serial.println("Humidity sent: " + http.getString());
  else
    Serial.println("❌ Lỗi gửi humidity");

  http.end();
}

void sendToServer(float voltage, float current, float power, float energy, float frequency, float pf) {
  HTTPClient http;
  http.begin(serverPower);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> doc;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["frequency"] = frequency;
  doc["power_factor"] = pf;

  String json;
  serializeJson(doc, json);
  int code = http.POST(json);

  if (code > 0)
    Serial.println("Power data sent: " + http.getString());
  else
    Serial.printf("❌ Lỗi gửi power data: %d\n", code);

  http.end();
}

// ================== ĐẢM BẢO WIFI ==================
void ensureWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔄 Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n🔁 Reconnected to WiFi!");
  }
}

// ================== TASK GỬI DỮ LIỆU ĐỊNH KỲ ==================
void dataTask(void * parameter) {
  const unsigned long interval = 10000;
  unsigned long lastSend = 0;

  while (true) {
    if (millis() - lastSend >= interval) {
      lastSend = millis();
      ensureWiFi();

      float t = dht.readTemperature();
      float h = dht.readHumidity();
      if (!isnan(t) && !isnan(h)) sendHumidity(t, h);

      float voltage = pzem.voltage();
      float current = pzem.current();
      float power = pzem.power();
      float energy = pzem.energy();
      float frequency = pzem.frequency();
      float pf = pzem.pf();

      sendToServer(voltage, current, power, energy, frequency, pf);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

  setup_wifi();

  pzemSerial.begin(9600, SERIAL_8N1, 16, 17);
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  SinricProSwitch &myLight1 = SinricPro[DEVICE_ID_1];
  myLight1.onPowerState(onPowerState1);
  SinricProSwitch &myLight2 = SinricPro[DEVICE_ID_2];
  myLight2.onPowerState(onPowerState2);
  SinricPro.begin(APP_KEY, APP_SECRET);

  xTaskCreatePinnedToCore(dataTask, "Data Sender", 4096, NULL, 1, NULL, 1);
}

// ================== LOOP ==================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  SinricPro.handle();
}

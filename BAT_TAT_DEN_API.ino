#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// ✅ Cấu hình WiFi
const char* ssid = "realme 8";
const char* password = "00001111";

// ✅ Thông tin Sinric Pro
#define APP_KEY     "8e10eb5d-2f08-4317-908b-322a540c6239"  
#define APP_SECRET  "07f2aa77-e000-4d17-aed1-c6af2b2b8e37-51b7fd9f-232a-4fdc-83a8-833818b01bf5"
#define DEVICE_ID_1 "67d13b264dee339ca79b6f80"  // Đèn 1
#define DEVICE_ID_2 "67d13b566066f22be0656db5"  // Đèn 2

// ✅ Định nghĩa chân Relay
#define RELAY_1 33
#define RELAY_2 32

// 📌 Xử lý lệnh bật/tắt đèn từ Sinric Pro
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
    
    // ✅ Kết nối WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");
    
    // ✅ Cấu hình chân Relay
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    digitalWrite(RELAY_1, LOW);
    digitalWrite(RELAY_2, LOW);

    // ✅ Cấu hình thiết bị trên Sinric Pro
    SinricProSwitch &myLight1 = SinricPro[DEVICE_ID_1];
    myLight1.onPowerState(onPowerState1);

    SinricProSwitch &myLight2 = SinricPro[DEVICE_ID_2];
    myLight2.onPowerState(onPowerState2);

    SinricPro.begin(APP_KEY, APP_SECRET);
}

void loop() {
    SinricPro.handle();
}

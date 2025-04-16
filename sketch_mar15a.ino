#include <DHT.h>
#include <DHT_U.h>
#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PZEM004Tv30.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "realme 8";
const char* password = "00001111";
const char* serverName = "http://192.168.119.183:5000/data";
const char* serverURL = "http://192.168.119.183:5000/power-data";  

HardwareSerial pzemSerial(1);
PZEM004Tv30 pzem(pzemSerial, 16, 17);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting...");
    }
    Serial.println("Connected to WiFi!");
    dht.begin();
}

void loop() {
    checkWiFiConnection(); // Kiểm tra WiFi trước khi gửi dữ liệu
    float voltage = pzem.voltage();
    if(voltage != NAN){
        Serial.print("Voltage: "); Serial.print(voltage); Serial.println("V");
    } else {
        Serial.println("Error reading voltage");
    }

    float current = pzem.current();
    if (!isnan(current)) {
        Serial.print("Current: "); Serial.print(current); Serial.println("A");
    } else {
        Serial.println("Error reading current");
    }

    float power = pzem.power();
    if (!isnan(power)) {
        Serial.print("Power: "); Serial.print(power); Serial.println("W");
    } else {
        Serial.println("Error reading power");
    }

    float energy = pzem.energy();
    if (!isnan(energy)) {
        Serial.print("Energy: "); Serial.print(energy,3); Serial.println("kWh");
    } else {
        Serial.println("Error reading energy");
    }

    float frequency = pzem.frequency();
    if (!isnan(frequency)) {
        Serial.print("Frequency: "); Serial.print(frequency, 1); Serial.println("Hz");
    } else {
        Serial.println("Error reading frequency");
    }

    float pf = pzem.pf();
    if (!isnan(pf)) {
        Serial.print("PF: "); Serial.println(pf);
    } else {
        Serial.println("Error reading power factor");
    }

    sendHumidity();
    sendToServer(voltage, current, power, energy, frequency, pf);

    delay(5000);
}

void sendHumidity() {
    if (WiFi.status() == WL_CONNECTED) {
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("DHT sensor error!");
            return;
        }

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
            String response = http.getString();
            Serial.println("Response from API: " + response);
        } else {
            Serial.println("Error sending data");
        }
        http.end();
    }
}

void sendToServer(float voltage, float current, float power, float energy, float frequency, float pf) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, skipping HTTP request!");
        return;
    }

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
    Serial.println("Sending data to server...");
    
    int httpResponseCode = http.POST(jsonStr);
    if (httpResponseCode > 0) {
        Serial.printf("Data sent! Response: %s\n", http.getString().c_str());
    } else {
        Serial.printf("HTTP error: %d\n", httpResponseCode);
        Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print(".");
        }
        Serial.println("\nReconnected to WiFi!");
    }
}

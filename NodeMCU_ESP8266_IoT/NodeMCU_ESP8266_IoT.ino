#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* OWM_API_KEY = "YOUR_OPENWEATHERMAP_KEY";
const char* OWM_CITY = "Islamabad,PK";

const char* TS_API_KEY = "YOUR_THINGSPEAK_WRITE_KEY";

SoftwareSerial atmegaSerial(D5, D6);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 18000, 60000);

int azDeg = 90;
int elDeg = 90;
int ldrTL = 0;
int ldrTR = 0;
int ldrBL = 0;
int ldrBR = 0;
int lightPct = 0;
int movedFlag = 0;
int trackerMode = 0;

char weatherMain[16] = "Clear";
float weatherTemp = 0.0f;
int weatherHum = 0;
float weatherWind = 0.0f;
int weatherAlert = 0;

char rxBuf[80];
uint8_t rxIdx = 0;

unsigned long lastWeatherMs = 0;
unsigned long lastCloudMs = 0;
unsigned long lastLcdMs = 0;
uint8_t lcdPage = 0;

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint8_t tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 30) {
        delay(500);
        tries++;
    }
}

void readAtmegaTelemetry() {
    while (atmegaSerial.available() > 0) {
        char c = (char)atmegaSerial.read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            rxBuf[rxIdx] = '\0';
            if (rxIdx > 1 && rxBuf[0] == '$') {
                sscanf(rxBuf + 1, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       &azDeg, &elDeg, &ldrTL, &ldrTR, &ldrBL, &ldrBR,
                       &lightPct, &movedFlag, &trackerMode);
            }
            rxIdx = 0;
        } else if (rxIdx < sizeof(rxBuf) - 1) {
            rxBuf[rxIdx++] = c;
        }
    }
}

void fetchWeatherAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    WiFiClient client;
    HTTPClient http;
    char url[180];
    snprintf(url, sizeof(url),
             "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric",
             OWM_CITY, OWM_API_KEY);

    if (http.begin(client, url)) {
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            if (!deserializeJson(doc, payload)) {
                const char* cond = doc["weather"][0]["main"] | "Clear";
                strncpy(weatherMain, cond, sizeof(weatherMain) - 1);
                weatherMain[sizeof(weatherMain) - 1] = '\0';
                weatherTemp = doc["main"]["temp"] | 0.0f;
                weatherHum = doc["main"]["humidity"] | 0;
                weatherWind = doc["wind"]["speed"] | 0.0f;

                if (strcmp(weatherMain, "Rain") == 0 ||
                    strcmp(weatherMain, "Thunderstorm") == 0 ||
                    strcmp(weatherMain, "Drizzle") == 0 ||
                    strcmp(weatherMain, "Squall") == 0 ||
                    weatherWind >= 12.0f) {
                    weatherAlert = 1;
                    atmegaSerial.write('R');
                } else {
                    weatherAlert = 0;
                    atmegaSerial.write('C');
                }
            }
        }
        http.end();
    }
}

void uploadThingSpeak() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    WiFiClient client;
    HTTPClient http;
    char url[256];
    snprintf(url, sizeof(url),
             "http://api.thingspeak.com/update?api_key=%s"
             "&field1=%d&field2=%d&field3=%d&field4=%d"
             "&field5=%d&field6=%.1f&field7=%.1f&field8=%d",
             TS_API_KEY, azDeg, elDeg, lightPct, movedFlag,
             trackerMode, weatherTemp, weatherWind, weatherAlert);

    if (http.begin(client, url)) {
        http.GET();
        http.end();
    }
}

void updateLCD() {
    char line0[17];
    char line1[17];

    lcd.clear();
    if (lcdPage == 0) {
        snprintf(line0, sizeof(line0), "AZ:%3d EL:%3d", azDeg, elDeg);
        snprintf(line1, sizeof(line1), "L:%3d%% %s M:%d",
                 lightPct, trackerMode ? "SAFE" : "TRK ", movedFlag);
    } else {
        time_t epoch = (time_t)timeClient.getEpochTime();
        struct tm* t = gmtime(&epoch);
        snprintf(line0, sizeof(line0), "%02d:%02d %02d/%02d/%02d",
                 t->tm_hour, t->tm_min, t->tm_mday, t->tm_mon + 1, (t->tm_year + 1900) % 100);
        snprintf(line1, sizeof(line1), "%.6s %.0fC W%.1f",
                 weatherMain, weatherTemp, weatherWind);
    }

    lcd.setCursor(0, 0);
    lcd.print(line0);
    lcd.setCursor(0, 1);
    lcd.print(line1);
    lcdPage ^= 1;
}

void setup() {
    Serial.begin(115200);
    atmegaSerial.begin(9600);

    Wire.begin(D2, D1);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Solar Tracker");
    lcd.setCursor(0, 1);
    lcd.print("Connecting WiFi");

    connectWiFi();
    timeClient.begin();
    timeClient.update();
    fetchWeatherAPI();
}

void loop() {
    unsigned long now = millis();

    readAtmegaTelemetry();

    if (now - lastWeatherMs >= 60000UL) {
        lastWeatherMs = now;
        connectWiFi();
        timeClient.update();
        fetchWeatherAPI();
    }

    if (now - lastCloudMs >= 20000UL) {
        lastCloudMs = now;
        uploadThingSpeak();
    }

    if (now - lastLcdMs >= 3000UL) {
        lastLcdMs = now;
        updateLCD();
    }
}

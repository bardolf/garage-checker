/* Garage checker */
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <FS.h>

#include "Adafruit_PCD8544.h"
#include "NewPingESP8266.h"

const char* timeHost = "worldclockapi.com";
const char* timeUrl = "/api/json/cet/now";
const int timePort = 80;
String lastReportedTime;
char localIP[16];

// const char* iftttHost = "";
const int iftttPort = 443;
const char* garageOpenedUrl = "https://maker.ifttt.com/trigger/garage_door_opened/with/key/";
const char* garageClosedUrl = "https://maker.ifttt.com/trigger/garage_door_closed/with/key/";
const char* iftttFingerprint = "AA:75:CB:41:2E:D5:F9:97:FF:5D:A0:8B:7D:AC:12:21:08:4B:00:8C";

// LCD pins
const int8_t RST_PIN = D2;
const int8_t CE_PIN = D1;
const int8_t DC_PIN = D6;
const int8_t BL_PIN = D0;
// Ultrasonic sensor pins
const int8_t TRIG_PIN = D8;
const int8_t ECHO_PIN = D3;

const int MAX_DISTANCE = 200;
const int COUNT_TO_SWITCH = 20;

const int STATE_DOOR_UNKNOWN = 1;
const int STATE_DOOR_CLOSED = 2;
const int STATE_DOOR_OPENED = 3;
int state = STATE_DOOR_UNKNOWN;

int count = 0;

struct Config {
    char ssid[32];
    char password[32];
    char ifftApiKey[32];
};

Config config;

NewPingESP8266 sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_PCD8544 display = Adafruit_PCD8544(DC_PIN, CE_PIN, RST_PIN);
HTTPClient http;

String getTime()
{
    String time = "??:??";
    if (WiFi.status() == WL_CONNECTED) {
        http.begin(timeHost, timePort, timeUrl);
        int httpCode = http.GET();
        Serial.printf("Time request http response code %d\n", httpCode);
        if (httpCode) {
            if (httpCode == 200) {
                String payload = http.getString();
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);
                const char* x = root["currentDateTime"];
                time = String(x).substring(11, 16);
            }
        }
        http.end();
    }
    return time;
}

void reportClosedDoor()
{
    digitalWrite(BUILTIN_LED, HIGH);
    digitalWrite(BL_PIN, LOW);
    if (WiFi.status() == WL_CONNECTED) {
        char url[128];
        sprintf(url, "%s%s", garageClosedUrl, config.ifftApiKey);
        http.begin(url, iftttFingerprint);
        int httpCode = http.GET();
        // Serial.printf("IFTTT request http response code %d\n", httpCode);
        http.end();
    }
    lastReportedTime = getTime();
}

void reportOpenedDoor()
{
    digitalWrite(BUILTIN_LED, LOW);
    digitalWrite(BL_PIN, HIGH);
    if (WiFi.status() == WL_CONNECTED) {
        char url[128];
        sprintf(url, "%s%s", garageOpenedUrl, config.ifftApiKey);
        http.begin(url, iftttFingerprint);
        int httpCode = http.GET();
        // Serial.printf("IFTTT request http response code %d\n", httpCode);
        http.end();
    }
    lastReportedTime = getTime();
}

void parseConfiguration()
{
    StaticJsonBuffer<512> jsonBuffer;
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    File file = SPIFFS.open("/configuration.json", "r");
    if (!file) {
        Serial.println("There was an error opening the file configuration.json");
        return;
    }
    JsonObject& root = jsonBuffer.parseObject(file);
    file.close();
    strlcpy(config.ssid, root["ssid"], sizeof(config.ssid));
    strlcpy(config.password, root["password"], sizeof(config.password));
    strlcpy(config.ifftApiKey, root["ifttt_key"], sizeof(config.ifftApiKey));
}

void setup()
{
    Serial.begin(9600);
    Serial.printf("Garage checker\n");

    //Turn off the built in led
    pinMode(BUILTIN_LED, OUTPUT); // initialize onboard LED as output
    digitalWrite(BUILTIN_LED, HIGH);

    //Initialize display - contrast and back light
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, LOW);
    display.begin();
    display.setContrast(60);

    parseConfiguration();

    WiFi.begin(config.ssid, config.password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    sprintf(localIP, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
}

void loop()
{
    int distance = sonar.ping_cm();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK);

    //show distance
    display.setCursor(0, 0);
    display.printf("Distance %dcm", distance);
    //show door state
    display.setCursor(0, 10);
    if (state == STATE_DOOR_OPENED) {
        display.printf("Opened (%d/%d)", count, COUNT_TO_SWITCH);
    } else if (state == STATE_DOOR_CLOSED) {
        display.printf("Closed (%d/%d)", count, COUNT_TO_SWITCH);
    } else {
        display.printf("Unknown(%d/%d)", count, COUNT_TO_SWITCH);
    }
    //show localIP
    display.setCursor(0, 20);
    display.printf(localIP);
    //show last reported time
    display.setCursor(0, 30);
    display.printf("Report %s", lastReportedTime.c_str());
    display.display();

    if ((state == STATE_DOOR_OPENED && distance != 0) || (state == STATE_DOOR_CLOSED && distance == 0) || (state == STATE_DOOR_UNKNOWN)) {
        count++;
    } else {
        count = 0;
    }

    if (count >= COUNT_TO_SWITCH) {
        if (state == STATE_DOOR_OPENED) {
            state = STATE_DOOR_CLOSED;
            reportClosedDoor();
        } else if (state == STATE_DOOR_CLOSED) {
            state = STATE_DOOR_OPENED;
            reportOpenedDoor();
        } else if (state == STATE_DOOR_UNKNOWN) {
            if (distance == 0) {
                state = STATE_DOOR_OPENED;
                reportOpenedDoor();
            } else {
                state = STATE_DOOR_CLOSED;
                reportClosedDoor();
            }
        }
    }
    delay(500);
}

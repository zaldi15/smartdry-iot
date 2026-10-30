
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include "HX711.h"
#include <math.h>

// ========================================
// SMARTDRY - SENSOR HEALTH & FAULT DETECTION
// ========================================

// WIFI
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "smartdry/zaldi/monitoring";

WiFiClient espClient;
PubSubClient client(espClient);

// DHT22
const int DHT_PIN = 15;
DHTesp dhtSensor;

// HX711 LOAD CELL
const int HX711_DT = 19;
const int HX711_SCK = 18;

HX711 scale;
const float calibration_factor = 420.0f;

// LED PWM
const int LED_LOW = 25;
const int LED_MEDIUM = 26;
const int LED_HIGH = 27;

// PWM VALUE
const int PWM_LOW = 77;
const int PWM_MEDIUM = 153;
const int PWM_HIGH = 255;

// BATAS SUHU SIMULASI
const float TEMP_WARNING = 45.0;
const float TEMP_CRITICAL = 50.0;

// INTERVAL
unsigned long lastSend = 0;
const unsigned long interval = 5000;

// ========================================
// WIFI CONNECTION
// ========================================
void setup_wifi() {

  Serial.print("Connecting WiFi");

  WiFi.begin(ssid, password, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
}

// ========================================
// MQTT CONNECTION
// ========================================
void reconnect_mqtt() {

  while (!client.connected()) {

    if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();
    }

    String clientId = "SmartDry-Health-";
    clientId += String(random(0xffff), HEX);

    Serial.print("Connecting MQTT... ");

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected!");
    } else {
      Serial.print("Failed: ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

// ========================================
// STOP ALL LED INDICATORS
// ========================================
void stopFanIndicators() {

  analogWrite(LED_LOW, 0);
  analogWrite(LED_MEDIUM, 0);
  analogWrite(LED_HIGH, 0);
}

// ========================================
// AUTOMATIC FAN & SAFETY CONTROL
// ========================================
void controlFanSafety(
  float temperature,
  float humidity,
  const char* &fan_speed,
  int &pwmValue,
  int &pwmPercent,
  const char* &safety_status,
  const char* &safety_action
) {

  stopFanIndicators();

  // TEMPERATURE CRITICAL
  if (temperature >= TEMP_CRITICAL) {

    safety_status = "CRITICAL";
    safety_action = "COOLING_OVERRIDE";

    fan_speed = "HIGH";
    pwmValue = PWM_HIGH;
    pwmPercent = 100;

    analogWrite(LED_HIGH, pwmValue);
  }

  // TEMPERATURE WARNING
  else if (temperature >= TEMP_WARNING) {

    safety_status = "WARNING";
    safety_action = "COOLING_OVERRIDE";

    fan_speed = "HIGH";
    pwmValue = PWM_HIGH;
    pwmPercent = 100;

    analogWrite(LED_HIGH, pwmValue);
  }

  // NORMAL TEMPERATURE
  else {

    safety_status = "NORMAL";
    safety_action = "NORMAL_OPERATION";

    if (humidity >= 60.0) {

      fan_speed = "HIGH";
      pwmValue = PWM_HIGH;
      pwmPercent = 100;

      analogWrite(LED_HIGH, pwmValue);
    }

    else if (humidity >= 50.0) {

      fan_speed = "MEDIUM";
      pwmValue = PWM_MEDIUM;
      pwmPercent = 60;

      analogWrite(LED_MEDIUM, pwmValue);
    }

    else {

      fan_speed = "LOW";
      pwmValue = PWM_LOW;
      pwmPercent = 30;

      analogWrite(LED_LOW, pwmValue);
    }
  }
}

// ========================================
// SETUP
// ========================================
void setup() {

  Serial.begin(115200);
  randomSeed(micros());

  pinMode(LED_LOW, OUTPUT);
  pinMode(LED_MEDIUM, OUTPUT);
  pinMode(LED_HIGH, OUTPUT);

  stopFanIndicators();

  // DHT22
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // HX711
  scale.begin(HX711_DT, HX711_SCK);
  scale.set_scale(calibration_factor);

  if (scale.wait_ready_timeout(2000)) {
    scale.tare(10);
    Serial.println("HX711 Ready!");
  } else {
    Serial.println("WARNING: HX711 not ready!");
  }

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(60);

  // Buffer diperbesar untuk tambahan status sensor
  client.setBufferSize(512);

  Serial.println("==============================");
  Serial.println("SMARTDRY SENSOR HEALTH SYSTEM");
  Serial.println("==============================");
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  if (millis() - lastSend >= interval) {

    lastSend = millis();

    // ====================================
    // READ DHT22
    // ====================================
    TempAndHumidity data =
      dhtSensor.getTempAndHumidity();

    float temperature = data.temperature;
    float humidity = data.humidity;

    bool dhtOK =
      isfinite(temperature) &&
      isfinite(humidity) &&
      temperature >= -40.0 &&
      temperature <= 80.0 &&
      humidity >= 0.0 &&
      humidity <= 100.0;

    const char* dht_status =
      dhtOK ? "OK" : "ERROR";

    // ====================================
    // READ HX711
    // ====================================
    bool hx711OK =
      scale.wait_ready_timeout(500);

    float water_mass = NAN;

    if (hx711OK) {

      water_mass = scale.get_units(3);

      if (!isfinite(water_mass)) {
        hx711OK = false;
      } else if (water_mass < 0) {
        water_mass = 0;
      }
    }

    const char* hx711_status =
      hx711OK ? "OK" : "ERROR";

    // ====================================
    // SENSOR HEALTH
    // ====================================
    const char* sensor_health = "OK";

    if (!dhtOK && !hx711OK) {
      sensor_health = "MULTIPLE_ERRORS";
    }

    else if (!dhtOK) {
      sensor_health = "DHT22_ERROR";
    }

    else if (!hx711OK) {
      sensor_health = "HX711_ERROR";
    }

    // ====================================
    // FAN & SAFETY CONTROL
    // ====================================
    const char* fan_speed = "OFF";
    const char* safety_status = "NORMAL";
    const char* safety_action = "NORMAL_OPERATION";

    int pwmValue = 0;
    int pwmPercent = 0;

    if (dhtOK) {

      controlFanSafety(
        temperature,
        humidity,
        fan_speed,
        pwmValue,
        pwmPercent,
        safety_status,
        safety_action
      );

    } else {

      // Penghentian indikator simulasi
      stopFanIndicators();

      safety_status = "SENSOR_ERROR";
      safety_action = "OUTPUT_DISABLED";

      fan_speed = "OFF";
      pwmValue = 0;
      pwmPercent = 0;
    }

    // ====================================
    // SERIAL MONITOR
    // ====================================
    Serial.println("------------------------------");

    Serial.print("DHT22 Status  : ");
    Serial.println(dht_status);

    Serial.print("HX711 Status  : ");
    Serial.println(hx711_status);

    Serial.print("Sensor Health : ");
    Serial.println(sensor_health);

    if (dhtOK) {
      Serial.print("Temperature   : ");
      Serial.print(temperature, 1);
      Serial.println(" C");

      Serial.print("Humidity      : ");
      Serial.print(humidity, 1);
      Serial.println(" %");
    } else {
      Serial.println("Temperature   : INVALID");
      Serial.println("Humidity      : INVALID");
    }

    if (hx711OK) {
      Serial.print("Load Mass     : ");
      Serial.print(water_mass, 3);
      Serial.println(" kg");
    } else {
      Serial.println("Load Mass     : INVALID");
    }

    Serial.print("Fan Speed     : ");
    Serial.println(fan_speed);

    Serial.print("PWM Value     : ");
    Serial.println(pwmValue);

    Serial.print("PWM Duty      : ");
    Serial.print(pwmPercent);
    Serial.println(" %");

    Serial.print("Safety        : ");
    Serial.println(safety_status);

    Serial.print("Action        : ");
    Serial.println(safety_action);

    // ====================================
    // CONVERT SENSOR VALUES TO JSON
    // ====================================
    char tempJson[24];
    char humidityJson[24];
    char massJson[24];

    if (dhtOK) {
      snprintf(tempJson, sizeof(tempJson),
               "%.1f", temperature);

      snprintf(humidityJson, sizeof(humidityJson),
               "%.1f", humidity);
    } else {
      snprintf(tempJson, sizeof(tempJson), "null");
      snprintf(humidityJson, sizeof(humidityJson), "null");
    }

    if (hx711OK) {
      snprintf(massJson, sizeof(massJson),
               "%.3f", water_mass);
    } else {
      snprintf(massJson, sizeof(massJson), "null");
    }

    // ====================================
    // MQTT JSON PAYLOAD
    // ====================================
    char payload[512];

    int length = snprintf(
      payload,
      sizeof(payload),

      "{"
      "\"temperature\":%s,"
      "\"humidity\":%s,"
      "\"water_mass\":%s,"
      "\"fan_speed\":\"%s\","
      "\"fan_pwm\":%d,"
      "\"safety_status\":\"%s\","
      "\"safety_action\":\"%s\","
      "\"sensor_health\":\"%s\","
      "\"dht_status\":\"%s\","
      "\"hx711_status\":\"%s\""
      "}",

      tempJson,
      humidityJson,
      massJson,
      fan_speed,
      pwmPercent,
      safety_status,
      safety_action,
      sensor_health,
      dht_status,
      hx711_status
    );

    if (length <= 0 ||
        length >= (int)sizeof(payload)) {

      Serial.println("ERROR: JSON payload too large!");
      return;
    }

    // ====================================
    // MQTT PUBLISH
    // ====================================
    bool sent =
      client.publish(mqtt_topic, payload);

    if (sent) {
      Serial.print("MQTT Sent: ");
      Serial.println(payload);
    } else {
      Serial.println("ERROR: MQTT Publish Failed!");
    }
  }

  delay(10);
}

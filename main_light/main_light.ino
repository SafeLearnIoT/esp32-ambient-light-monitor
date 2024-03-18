#include "globals.h"
#define LIGHT_SENSOR_PIN A1

void setup() {
  Serial.begin(115200);
  while (!Serial);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  mqtt_setup();
  pinMode(LIGHT_SENSOR_PIN, INPUT);
}

void loop() {
  mqtt_client.loop();
  delay(10);

  if (!mqtt_client.connected()) {
    mqtt_connect();
  }

  auto analog_read = analogRead(LIGHT_SENSOR_PIN);
  auto resistance = static_cast<float>(1023-analog_read)*10/analog_read;
  
  data.clear();
  String topic = "/esp32/";
  topic += TYPE;
  String output;
  data["timestamp"] = millis();
  data["analog_read"] = analog_read;
  data["resistance"] = resistance;

  serializeJson(data, output);

  mqtt_client.publish(topic.c_str(), output.c_str());

  delay(10000);
}

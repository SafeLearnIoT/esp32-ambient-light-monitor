#include <Arduino.h>

#include "env.h"
#include "ArduinoJson.h"
#include "communication.h"

const char ssid[] = SSID_ENV;
const char pass[] = PASSWORD_ENV;

const char mqtt_host[] = MQTT_HOST_ENV;
const int mqtt_port = MQTT_PORT_ENV;

JsonDocument data;

#define LIGHT_SENSOR_PIN A1

void callback(String &topic, String &payload)
{
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(", Payload: ");
  Serial.println(payload);
}

Communication comm(SSID_ENV, PASSWORD_ENV, "/esp32/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  while (!Serial)
    ;
  comm.setup();
}

void loop()
{
  comm.handle_mqtt_loop();

  auto analog_read = analogRead(LIGHT_SENSOR_PIN);
  auto resistance = static_cast<float>(1023 - analog_read) * 10 / analog_read;

  data.clear();

  data["timestamp"] = millis();
  data["analog_read"] = analog_read;
  data["resistance"] = resistance;

  String output;
  serializeJson(data, output);
  comm.publish("grove_light_sensor", output);

  delay(10000);
}

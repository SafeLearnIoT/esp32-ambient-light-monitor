#define XIAO
#include <Arduino.h>

#include "env.h"
#include "communication.h"
#include "ml.h"

#define LIGHT_SENSOR_PIN A1

auto analog_read_ml = ML(Light, "analog_read");

unsigned long lastDataSaveMillis = 0;

void callback(String &topic, String &payload);

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void callback(String &topic, String &payload)
{
    Serial.println("[SUB][" + topic + "] " + payload);
    if(payload == "get_weights")
    {
        comm->hold_connection();
        comm->publish("cmd", "", true);
        auto weights = analog_read_ml.get_weights();
        serializeJson(weights, Serial);
        Serial.println();
    }
    if(payload=="set_weights")
    {
        comm->release_connection();
    }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  comm->setup();
}

void loop()
{
  comm->handle_mqtt_loop();
  if (millis() - lastDataSaveMillis > 900000) // 60000 - minute
  {
    comm->resume_communication();
    lastDataSaveMillis = millis();
    auto analog_read = analogRead(LIGHT_SENSOR_PIN);
    auto raw_time = comm->get_rawtime();

    JsonDocument sensor_data;
    sensor_data["time_sent"] = raw_time;
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["analog_read"] = analog_read;


    analog_read_ml.perform(analog_read);
    
    serializeJson(sensor_data, Serial);
    comm->send_data(sensor_data);
  }
}

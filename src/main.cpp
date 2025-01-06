#define XIAO
#include <Arduino.h>

#include "env.h"
#include "communication.h"
#include "ml.h"

#define LIGHT_SENSOR_PIN A1
unsigned long lastDataUploadMillis = 0;
void callback(String &topic, String &payload)
{
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(", Payload: ");
  Serial.println(payload);
}

unsigned long lastDataSaveMillis = 0;

auto ml_algo = MLAlgo::None;
auto analog_read_ml = ML(Light, ml_algo, "analog_read");

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  comm->setup();

  delay(5000);

  comm->pause_communication();
}

void loop()
{
  if (millis() - lastDataSaveMillis > 900000) // 60000 - minute
  {
    comm->resume_communication();
    lastDataSaveMillis = millis();
    auto analog_read = analogRead(LIGHT_SENSOR_PIN);
    auto raw_time = comm->get_rawtime();
    auto time_struct = localtime(&raw_time);

    JsonDocument sensor_data;
    sensor_data["time_sent"] = raw_time;
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["analog_read"] = analog_read;

    JsonDocument ml_data;

    if (ml_algo != MLAlgo::None)
    {
      ml_data["time_sent"] = raw_time;
      ml_data["device"] = comm->get_client_id();
      ml_data["ml_algo"] = algoString[ml_algo];
      JsonObject data = ml_data["data"].to<JsonObject>();

      analog_read_ml.perform(*time_struct, analog_read, data);
    }

    comm->send_data(sensor_data, ml_data);
  }
}

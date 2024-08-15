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

ML *analog_read_ml;

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  comm->setup();

  delay(5000);

  comm->pause_communication();

  switch (comm->get_ml_algo())
  {
  case MLAlgo::LinReg:
    analog_read_ml = new Regression::Linear(SensorType::Light);
    break;
  case MLAlgo::LogReg:
    analog_read_ml = new Regression::Logistic(SensorType::Light);
    break;
  case MLAlgo::rTPNN:
    analog_read_ml = new RTPNN::SDP(SensorType::Light);
    break;
  case MLAlgo::None:
    break;
  }
}

void loop()
{
  if (millis() - lastDataSaveMillis > 60000) // 60000 - minute
  {
    lastDataSaveMillis = millis();
    auto analog_read = analogRead(LIGHT_SENSOR_PIN);
    auto raw_time = comm->get_rawtime();
    auto time_struct = localtime(&raw_time);

    JsonDocument sensor_data;
    sensor_data["time"] = raw_time;
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["analog_read"] = analog_read;

    JsonDocument ml_data;

    if (comm->get_ml_algo() != MLAlgo::None)
    {
      ml_data["time"] = raw_time;
      ml_data["device"] = comm->get_client_id();
      ml_data["ml_algo"] = "rtpnn";
      JsonObject detail_data = ml_data["data"].to<JsonObject>();

      JsonObject analog_read_data = detail_data["analog_read"].to<JsonObject>();
      analog_read_data = analog_read_ml->perform(*time_struct, analog_read);
    }

    comm->send_data(sensor_data, ml_data);
  }
}

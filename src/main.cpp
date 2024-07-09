#define XIAO
#include <Arduino.h>

#include "env.h"
#include "communication.h"
#include "rtpnn.h"
#include "data.h"

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

rTPNN::SDP<uint16_t> analog_read_sdp(rTPNN::SDPType::Light);

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

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
  if (millis() - lastDataSaveMillis > 60000) // 600000) // 60000 - minute
  {
    lastDataSaveMillis = millis();
    auto analog_read = analogRead(LIGHT_SENSOR_PIN);

    JsonDocument sensor_data;
    sensor_data["time"] = comm->get_rawtime();
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["analog_read"] = analog_read;

    JsonDocument rtpnn_data;
    rtpnn_data["time"] = comm->get_rawtime();
    rtpnn_data["device"] = comm->get_client_id();
    rtpnn_data["ml_algo"] = "rtpnn";
    JsonObject detail_rtpnn_data = rtpnn_data["data"].to<JsonObject>();

    JsonObject analog_read_data = detail_rtpnn_data["analog_read"].to<JsonObject>();
    auto analog_read_calc = analog_read_sdp.execute_sdp(analog_read);
    analog_read_data["trend"] = analog_read_calc.first;
    analog_read_data["level"] = analog_read_calc.second;

    JsonDocument reglin_data;

    comm->send_data(sensor_data, rtpnn_data, reglin_data);
  }
}

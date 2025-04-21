#define XIAO
#include <Arduino.h>

#include "jsonToArray.h"
#include "env.h"
#include "communication.h"
#include "ml.h"

#define LIGHT_SENSOR_PIN A1

unsigned long lastDataSaveMillis = 0;

void callback(String &topic, String &payload);
auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

auto analog_read_ml = ML(Light, "analog_read");

bool get_params = false;
bool set_params = false;
String payload_content;

void callback(String &topic, String &payload)
{
    if (topic == "configuration" && !comm->is_system_configured())
    {
        comm->initConfig(payload);
        return;
    }
    Serial.println("[SUB][" + topic + "] " + payload);
    if (payload == "get_params")
    {
        if (!get_params)
        {
            get_params = true;
        }
        return;
    }
    if (payload.startsWith("set_params;"))
    {
        if (!set_params)
        {
            payload_content = payload;
            set_params = true;
        }
        return;
    }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  comm->setup();

  delay(5000);
}

void loop()
{
  if (comm->is_system_configured())
  {
    if (get_params)
    {
      comm->hold_connection();

      JsonDocument doc;
      auto raw_time = comm->get_rawtime();

      doc["time_sent"] = raw_time;
      doc["device"] = comm->get_client_id();

      JsonObject data = doc["data"].to<JsonObject>();
      

      data["type"] = "params";
      analog_read_ml.get_params(data);
      comm->publish("cmd_mcu", "params;" + doc.as<String>());
      get_params = false;
    }
    if (set_params)
    {
      JsonDocument doc;
      if (payload_content.substring(11) == "ok")
      {
        comm->publish("cmd_gateway", "", true);
        comm->release_connection();
        payload_content.clear();
        set_params = false;
        return;
      }
      deserializeJson(doc, payload_content.substring(11));

      auto analog_read_weights = Converter<std::array<double, 8>>::fromJson(doc["analog_read"]);
      Serial.print("[Analog read]");
      analog_read_ml.set_params(analog_read_weights);

      // Clear message
      comm->publish("cmd_gateway", "", true);

      comm->release_connection();
      payload_content.clear();
      set_params = false;
    }

    comm->handle_mqtt_loop();
    
    if (millis() - lastDataSaveMillis > comm->m_configuration->getActionIntervalMillis()) // 60000 - minute
    {
      comm->resume_communication();
      lastDataSaveMillis = millis();
      auto analog_read = analogRead(LIGHT_SENSOR_PIN);
      auto raw_time = comm->get_rawtime();

      if (comm->m_configuration->getSendSensorData())
      {
        JsonDocument sensor_data;
        sensor_data["time_sent"] = raw_time;
        sensor_data["device"] = comm->get_client_id();
        JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
        detail_sensor_data["test_name"] = comm->m_configuration->getTestName();
        detail_sensor_data["analog_read"] = analog_read;

        comm->send_data(sensor_data);
      }

      if (comm->m_configuration->getRunMachineLearning())
      {
        JsonDocument ml_data;
        ml_data["time_sent"] = raw_time;
        ml_data["device"] = comm->get_client_id();
        JsonObject detail_ml_data = ml_data["data"].to<JsonObject>();
        detail_ml_data["type"] = "prediction";
        detail_ml_data["test_name"] = comm->m_configuration->getTestName();

        JsonObject analog_read_data = detail_ml_data["analog_read"].to<JsonObject>();

        auto training_mode = comm->m_configuration->getMachineLearningTrainingMode();
        analog_read_ml.perform(analog_read, analog_read_data, training_mode);
        comm->send_ml(ml_data);
      }
    }
  }
  else
  {
    Serial.println("System not configured");
    comm->handle_mqtt_loop();
    delay(1000);
  }
}

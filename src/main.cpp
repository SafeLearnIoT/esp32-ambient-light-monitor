#define XIAO
#include <Arduino.h>

#include "jsonToArray.h"
#include "env.h"
#include "communication.h"
#include "ml.h"

#define LIGHT_SENSOR_PIN A1

auto analog_read_ml = ML(Light, "analog_read");

unsigned long lastDataSaveMillis = 0;

void callback(String &topic, String &payload);

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

bool get_weights = false;
bool set_weights = false;
String payload_content;

void callback(String &topic, String &payload)
{
    Serial.println("[SUB][" + topic + "] " + payload);
    if(payload == "get_weights")
    {
        if(!get_weights){
            get_weights = true;
        }
    }
    if(payload.startsWith("set_weights;"))
    {
       if(!set_weights){
           payload_content = payload;
           set_weights = true;
       }
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
  if(get_weights)
    {
        comm->hold_connection();

        JsonDocument doc;
        analog_read_ml.get_weights(doc);
        comm->publish("cmd_mcu", "weights;" + doc.as<String>());
        get_weights = false;
    }
    if(set_weights)
    {   
        JsonDocument doc;
        if(payload_content.substring(12)=="ok"){
            comm->publish("cmd_gateway", "", true);
            comm->release_connection();
            payload_content.clear();
            set_weights = false;
            return;
        }
        deserializeJson(doc, payload_content.substring(12));
        
        auto analog_read_weights = Converter<std::array<double, 4>>::fromJson(doc["analog_read"]);
        Serial.print("[Analog read]");
        analog_read_ml.set_weights(analog_read_weights);

        // Clear message
        comm->publish("cmd_gateway", "", true);

        comm->release_connection();
        payload_content.clear();
        set_weights = false;
    }

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

    JsonDocument ml_data;
    ml_data["time_sent"] = raw_time;
    ml_data["device"] = comm->get_client_id();
    JsonObject detail_ml_data = ml_data["data"].to<JsonObject>();

    JsonObject analog_read_data = detail_ml_data["analog_read"].to<JsonObject>();
    analog_read_ml.perform(analog_read, analog_read_data);
    
    //serializeJson(sensor_data, Serial);
    //Serial.println();
    //comm->send_data(sensor_data);

    serializeJson(ml_data, Serial);
    Serial.println();
    comm->send_ml(ml_data);
  }
}

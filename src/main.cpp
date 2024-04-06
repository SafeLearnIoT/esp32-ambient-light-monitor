#define XIAO
#include <Arduino.h>

#include "env.h"
#include "communication.h"
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

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/light/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

unsigned long lastDataSaveMillis = 0;
bool upload_init = false;
String header = "timestamp,analog_read,resistance\n";

String get_todays_file_path()
{
  return "/" + comm->get_todays_date_string() + ".csv";
};

String get_yesterdays_file_path()
{
  return "/" + comm->get_yesterdays_date_string() + ".csv";
};

void read_data()
{
  auto analog_read = analogRead(LIGHT_SENSOR_PIN);
  auto resistance = static_cast<float>(1023 - analog_read) * 10 / analog_read;

  String output = String(comm->get_rawtime()) + ",";
  output += String(analog_read) + ",";
  output += String(resistance);
  output += "\n";
  if (millis() - lastDataSaveMillis > 900000) // 600000) // 60000 - minute
  {
    lastDataSaveMillis = millis();

    if (!SPIFFS.exists(get_todays_file_path()))
    {
      writeFile(SPIFFS, get_todays_file_path().c_str(), header.c_str());
    }
    appendFile(SPIFFS, get_todays_file_path().c_str(), output.c_str());
  }
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    Serial.println("SPIFFS Mount Failed");
  }

  pinMode(LIGHT_SENSOR_PIN, INPUT);

  comm->setup();
}

void loop()
{
  comm->handle_mqtt_loop();
  read_data();

  auto current_time = comm->get_localtime();
  if (current_time->tm_hour == 1 && !upload_init)
  {
    upload_init = true;
    comm->publish("data", readFile(SPIFFS, get_yesterdays_file_path().c_str()));
    checkAndCleanFileSystem(SPIFFS);
  }
  if (current_time->tm_hour == 2 && upload_init)
  {
    listDir(SPIFFS, "/", 0);
    upload_init = false;
  }

  // if (millis() - lastDataUploadMillis > 60000)
  // {
  //   lastDataUploadMillis = millis();
  //   if (SPIFFS.exists(get_todays_file_path()))
  //   {
  //     comm->publish("data", readFile(SPIFFS, get_todays_file_path().c_str()));
  //   }
  // }
}

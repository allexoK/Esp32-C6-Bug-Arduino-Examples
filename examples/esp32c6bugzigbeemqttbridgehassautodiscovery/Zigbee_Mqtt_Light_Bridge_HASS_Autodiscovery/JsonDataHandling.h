#include <ArduinoJson.h>
#include "SPIFFS.h" // SPIFFS storage
#include "FS.h"

#ifndef JSON_DATA_HANDLING_H
#define JSON_DATA_HANDLING_H
String mqttServer = "";
int mqttPort = 0;
String mqttUser = "";
String mqttPassword = "";

DynamicJsonDocument doc(2048);
JsonObject root = doc.to<JsonObject>();
DynamicJsonDocument docconf(1024);
JsonObject rootconf = docconf.to<JsonObject>();

bool saveZbDevicesToSPIFFS(void) {
//    Serial.println("saveZbDevicesToSPIFFS start");
    // Open file
    File file = SPIFFS.open("/zb_devices.json", FILE_WRITE);
    // If file not opened - return false
    if(!file) {
      Serial.println("Fail open zb devices file");
      return false;
    }
    // Write file start
    if(serializeJson(root, file) == 0) { // If write fail
      Serial.println("Fail save zb devices file");
      return false;
    } else { // If write success
//      Serial.println("saveZbDevicesToSPIFFS finish");
      return true;
    }
  }

bool loadZbDevicesFromSPIFFS(void) {
//    Serial.println("loadZbDevicesToSPIFFS start");
    // Open file
    File file = SPIFFS.open("/zb_devices.json");
    // If open file failed - return false
    if(!file || file.isDirectory()) {
      Serial.println("Fail open zb devices file");
      return false;
    }
    Serial.println("Deserialize start");
    deserializeJson(root, file);
    Serial.println("Deserialize finish");

    for (JsonPair kv : root) {
      if(root[kv.key().c_str()].containsKey("command_topic"))client.subscribe(root[kv.key().c_str()]["command_topic"]);
    }
//    Serial.println("loadZbDevicesToSPIFFS finish");    
    return true;
  }


bool saveConfigToSPIFFS(void) {
//    Serial.println("saveConfigToSPIFFS start");
    // Open file
    rootconf["mqttServer"] = mqttServer.c_str();
    rootconf["mqttPort"] = mqttPort;
    rootconf["mqttUser"] = mqttUser.c_str();
    rootconf["mqttPassword"] = mqttPassword.c_str();
      
    File file = SPIFFS.open("/mqttconf.json", FILE_WRITE);
    // If file not opened - return false
    if(!file) {
      Serial.println("Fail open conf file");
      return false;
    }
    // Write file start
    if(serializeJson(rootconf, file) == 0) { // If write fail
      Serial.println("Fail write conf file");
      return false;
    } else { // If write success
//      Serial.println("saveConfigToSPIFFS finish");
      return true;
    }
  }

bool loadConfigFromSPIFFS(void) {
//    Serial.println("loadConfigFromSPIFFS start");
    // Open file
    File file = SPIFFS.open("/mqttconf.json");
    // If open file failed - return false
    if(!file || file.isDirectory()) {
      Serial.println("Fail open file");
      return false;
    }
    deserializeJson(rootconf, file);
    
    mqttServer = String(rootconf["mqttServer"]);
    mqttPort = rootconf["mqttPort"];
    mqttUser = String(rootconf["mqttUser"]);
    mqttPassword = String(rootconf["mqttPassword"]);

//    Serial.println("loadConfigFromSPIFFS finish");    
    return true;
  }

#endif

  

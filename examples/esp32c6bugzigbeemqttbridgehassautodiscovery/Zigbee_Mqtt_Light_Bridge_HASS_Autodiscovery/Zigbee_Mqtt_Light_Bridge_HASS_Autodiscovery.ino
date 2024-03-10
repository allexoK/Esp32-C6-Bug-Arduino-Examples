#include "ZigbeeLight.h"
#include <SPI.h>
#include <ETH.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <LinkedList.h>
#include <WebServer.h>
#include <Ticker.h>

enum BuggyBridgeStates{
   NO_NETWORK = 0,
   BEGIN_CONFIG = 1,
   CONFIG = 2,
   END_CONFIG = 3,
   CONNECTING_TO_MQTT = 4,
   MQTT_BRIDGE_FUNCTIONING = 5,
   SET_MQTT_SERVER = 6,
  };
Ticker statusTicker;
BuggyBridgeStates buggyBridgeState = NO_NETWORK;
WiFiClient ethClient;
PubSubClient client(ethClient);
WebServer configServer(80);


#include "JsonDataHandling.h"
#include "BridgeCallbacks.h"
#include "WebConfig.h"

#define ETH_TYPE        ETH_PHY_W5500
#define ETH_ADDR         1
#define ETH_CS           5
#define ETH_IRQ          4
#define ETH_RST          -1
// SPI pins
#define ETH_SPI_SCK     6
#define ETH_SPI_MISO    2
#define ETH_SPI_MOSI    7

#define CONTROL_BUTTON  9
#define STATE_PIN       8

void toggleStateLed(void){
  digitalWrite(STATE_PIN, !digitalRead(STATE_PIN));
  }

void reconnect() {
  // Loop until we're reconnected
  Serial.print("Attempting MQTT connection...");
    // Attempt to connect
  if (client.connect("esp32c6bugClient",mqttUser.c_str(),mqttPassword.c_str())) {
    Serial.println("connected");
    loadZbDevicesFromSPIFFS();
    saveConfigToSPIFFS();
    buggyBridgeState = MQTT_BRIDGE_FUNCTIONING;
    statusTicker.detach();
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
      // Wait 5 seconds before retrying
   }
}

/********************* Arduino functions **************************/
void setup() {
    Serial.begin(115200);
    Serial.println("Let's go!");
    WiFi.onEvent(ethEventsCallbacks);
    
    if(!SPIFFS.begin(true)) {
      Serial.println("SPIFFs not working, continuing!");
    }

    loadConfigFromSPIFFS();
    
    client.setBufferSize(2048);
    client.setCallback(subsribedTopicNewMessageCallback);

    SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
    ETH.begin(ETH_TYPE, ETH_ADDR, ETH_CS, ETH_IRQ, ETH_RST, SPI);

    ZigbeeLight.set_device_find_callback(new_zigbee_device_found);
    ZigbeeLight.set_device_report_callback(attribute_report_callback);
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);

    configServer.on("/", handleRoot);
    configServer.on("/submit", handleSubmit);

    pinMode(CONTROL_BUTTON, INPUT);

    pinMode(STATE_PIN,OUTPUT);
    digitalWrite(STATE_PIN,1);
}

void loop() {
  switch (buggyBridgeState){
    case NO_NETWORK: {
      digitalWrite(STATE_PIN,1);
      break;
      }
    case SET_MQTT_SERVER:{
      Serial.print("Set MQTT server to ");
      Serial.println(mqttServer);
      if(client.connected())client.disconnect();
      client.setServer(mqttServer.c_str(), mqttPort);
      statusTicker.attach_ms(1000, toggleStateLed);
      // Allow the hardware to sort itself out
      delay(1500);
      buggyBridgeState=CONNECTING_TO_MQTT;
      break;
      }
    case CONNECTING_TO_MQTT:{
      reconnect();
      break;
      }
    case MQTT_BRIDGE_FUNCTIONING:{
        digitalWrite(STATE_PIN,0);
        if (client.connected())client.loop();
        else buggyBridgeState=CONNECTING_TO_MQTT;
      break;
      }
    case BEGIN_CONFIG:{
      Serial.print("Config server begin on ");
      Serial.println(ETH.localIP());
      configServer.begin();
      buggyBridgeState = CONFIG;
      statusTicker.attach_ms(100, toggleStateLed);
      break;
      }
    case CONFIG:{
      configServer.handleClient();
      break;
      }
    case END_CONFIG:{
      statusTicker.detach();
      configServer.stop();
      buggyBridgeState = SET_MQTT_SERVER;
      Serial.println("Config server end");
      break;
      }
    }
  if(digitalRead(CONTROL_BUTTON)==0 && buggyBridgeState!=CONFIG){
    buggyBridgeState = BEGIN_CONFIG;
    }
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

#include "Zigbee.h"
// #include <SPI.h>
// #include <ETH.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// #define USE_ETH

#ifdef USE_ETH
#include <SPI.h>
#include <ETH.h>
#define ETH_TYPE        ETH_PHY_W5500
#define ETH_ADDR         1
#define ETH_CS           5
#define ETH_IRQ          4
#define ETH_RST          -1

// SPI pins
#define ETH_SPI_SCK     6
#define ETH_SPI_MISO    2
#define ETH_SPI_MOSI    7
#else
const char* ssid     = "XXXXX"; // Change this to your WiFi SSID
const char* password = "XXXXX"; // Change this to your WiFi password
#endif

StaticJsonDocument<256> doc;

static bool eth_connected = false;
WiFiClient ethClient;
PubSubClient client(ethClient);
const char* mqttServer = "XXXXX";
const int mqttPort = 1883;
const char* mqttUser = "XXXXX";
const char* mqttPassword = "XXXXX";


static void esp_zb_task(void *pvParameters)
{
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZC_CONFIG();
    esp_zb_init(&zb_nwk_cfg);
    esp_zb_on_off_switch_cfg_t switch_cfg = ESP_ZB_DEFAULT_ON_OFF_SWITCH_CONFIG();
    esp_zb_ep_list_t *esp_zb_on_off_switch_ep = esp_zb_on_off_switch_ep_create(HA_ONOFF_SWITCH_ENDPOINT, &switch_cfg);
    esp_zb_device_register(esp_zb_on_off_switch_ep);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_main_loop_iteration();
}


void callback(char* topic, byte* payload, unsigned int length) {
  String payloadstr = String((char *)payload);
  int bstate = -1;
  if(length<7) {
    if(payload[0] == 48)bstate=0;
    if(payload[0] == 49)bstate=1;
  } else {
    deserializeJson(doc, (const byte*)payload, length);
    if(doc.containsKey("status")) {
      if(doc["status"] == 0)bstate=0;
      if(doc["status"] == 1)bstate=1;
    }
  }
  if(bstate != -1){
    if(bstate == 0 || bstate == 1){
      switch_func_pair_t button_func_pair;
      button_func_pair.state = bstate;
      button_func_pair.func = SWITCH_ONOFF_TOGGLE_CONTROL;
      (*esp_zb_buttons_handler)(&button_func_pair);
      }
    }
  // Handle button switch in loop()
}


void onEvent(arduino_event_id_t event, arduino_event_info_t info)
{
  switch (event) {
#ifdef USE_ETH
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-eth0");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("ETH Got IP: '%s'\n", esp_netif_get_desc(info.got_ip.esp_netif));
      ETH.printInfo(Serial);
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
#else
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WIFI_STA Started");
      //set eth hostname here
      WiFi.setHostname("esp32-wifi");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WIFI_STA Connected");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WIFI_STA Got IP: ");
      Serial.println(WiFi.localIP());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WIFI_STA Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WIFI_STA Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("WIFI_STA Stopped");
      eth_connected = false;
      break;
#endif
    default:
      break;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32c6bugClient",mqttUser,mqttPassword)) {
      Serial.println("connected");
      client.publish("test/status","light ready");
      client.subscribe("test/light");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

/********************* Arduino functions **************************/
void setup() {
    Serial.begin(115200);
    Serial.println("Let's go!");
    WiFi.onEvent(onEvent);

#ifdef USE_ETH
    SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
    ETH.begin(ETH_TYPE, ETH_ADDR, ETH_CS, ETH_IRQ, ETH_RST, SPI);
#else
    Serial.print("WiFi STA Connecting to ");
    Serial.println(ssid);

    // WiFi.setHostname("esp32-wifi0");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
#endif

    client.setServer(mqttServer, 1883);
    client.setCallback(callback);
    // Allow the hardware to sort itself out
    delay(1500);

    // Init Zigbee
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };

    ESP_ERROR_CHECK(esp_zb_platform_config(&config));



    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }else{
    client.loop();
//    client.publish("test/outTopic","device ready");
  }
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

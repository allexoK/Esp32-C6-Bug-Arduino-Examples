#include "JsonDataHandling.h"

#ifndef BRIDGE_CALLBACKS_H
#define BRIDGE_CALLBACKS_H

void subsribedTopicNewMessageCallback(char* topic, byte* payload, unsigned int length) {
  String topicstr = String((char*) topic);
  Serial.println(topicstr);
  Serial.println();
  uint16_t i;

  for (JsonPair kv : root) {
    if(root[kv.key().c_str()]["command_topic"] == topicstr){
      Serial.println("control bulb");
      String payloadstr = "";
      for (i=0;i<length;i++){
        payloadstr += (char)*(payload+i);
      }
      int bstate = -1;
      if(payloadstr == root[kv.key().c_str()]["payload_off"])bstate=0;
      if(payloadstr == root[kv.key().c_str()]["payload_on"])bstate=1;  
      if(bstate != -1){
        if(bstate == 0 || bstate == 1){
          switch_func_pair_t button_func_pair;
          button_func_pair.state = bstate;
          button_func_pair.short_addr = root[kv.key().c_str()]["buggylight_additional"]["zb_short_address"];
          button_func_pair.endpoint =root[kv.key().c_str()]["buggylight_additional"]["zb_endpoint"];
          
          button_func_pair.func = SWITCH_ONOFF_TOGGLE_CONTROL;
          (*esp_zb_buttons_handler)(&button_func_pair);
          }
        }
      }
  }
}

void attribute_report_callback(const esp_zb_zcl_report_attr_message_t *message){
  for (JsonPair kv : root) {
//    Serial.println(kv.key().c_str());
    if(root[kv.key().c_str()].containsKey("buggylight_additional")){
      if(root[kv.key().c_str()]["buggylight_additional"]["zb_short_address"]==message->src_address.u.short_addr){
        if(message->cluster == 6){
          String attribute_val = String(message->attribute.data.value ? *(uint8_t *)message->attribute.data.value : 0);
          if(client.connected())client.publish((char*)String(root[kv.key().c_str()]["state_topic"]).c_str(),(uint8_t*)attribute_val.c_str(),attribute_val.length());
          }
//        Serial.println("Short addr match");
        }
      }
    }
  }


void ethEventsCallbacks(arduino_event_id_t event, arduino_event_info_t info)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("buggyBridge");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.printf("ETH Got IP: '%s'\n", esp_netif_get_desc(info.got_ip.esp_netif));
      ETH.printInfo(Serial);
      if(mqttServer.length()!=0)buggyBridgeState = SET_MQTT_SERVER;
      else buggyBridgeState = BEGIN_CONFIG;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      buggyBridgeState = NO_NETWORK;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      buggyBridgeState = NO_NETWORK;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      buggyBridgeState = NO_NETWORK;
      break;
    default:
      break;
  }
}

void new_zigbee_device_found(uint16_t addr, uint8_t endpoint, esp_zb_ieee_addr_t ieee_addr){
  if (client.connected()) {
      char buffer[2048];
      String ieaddr = "";
      uint8_t i;
      for(i=0;i<8;i++){
        ieaddr+=String(ieee_addr[i],HEX);
        }
      String discoveryTopic = "homeassistant/switch/buggylight_" + String(ieaddr) + "/config";
 
      
     for (JsonPair kv : root) {
        if(kv.key().c_str() == "buggylight_" + ieaddr){
          root.remove("buggylight_" + ieaddr);
          }
      }

      JsonObject devicemqttobj = root.createNestedObject("buggylight_" + ieaddr);
      devicemqttobj["name"] = "buggylight_" + ieaddr;
      devicemqttobj["state_topic"]   = "buggylight_" + ieaddr+"/state";
      devicemqttobj["command_topic"]   = "buggylight_" + ieaddr+"/command";
      devicemqttobj["unique_id"] = "buggylight_" + ieaddr;
      devicemqttobj["payload_on"] = "1";
      devicemqttobj["payload_off"] = "0";
      JsonObject deviceaddconfobj = devicemqttobj.createNestedObject("buggylight_additional");
      deviceaddconfobj["zb_ieee"] = ieaddr;
      deviceaddconfobj["zb_short_address"] = addr;
      deviceaddconfobj["zb_endpoint"] = endpoint;      
      
      JsonObject device = devicemqttobj.createNestedObject("device");
      device["name"] = "buggylight"+ ieaddr;
      JsonArray identifiers = device.createNestedArray("identifiers");
      identifiers.add("buggylight"+ ieaddr);
      
      size_t n = serializeJson(devicemqttobj, buffer);

      String commandtpc=String("buggylight_"+ieaddr+"/command");
      client.subscribe(commandtpc.c_str());
      client.publish(discoveryTopic.c_str(),(uint8_t*)buffer,n, true);
      saveZbDevicesToSPIFFS();
    }  
  }

#endif

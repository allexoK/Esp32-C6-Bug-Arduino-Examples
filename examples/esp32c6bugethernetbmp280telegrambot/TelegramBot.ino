/*
  Karavaev Aleksei

  Project created using Brian Lough's Universal Telegram Bot Library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
  Example based on the Universal Arduino Telegram Bot Library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/blob/master/examples/ESP8266/FlashLED/FlashLED.ino

*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <SPI.h>
#include <ETH.h>
#include <Adafruit_BMP280.h>


#define ETH_TYPE        ETH_PHY_W5500
#define ETH_ADDR         1
#define ETH_CS           5
#define ETH_IRQ          4
#define ETH_RST          -1

// SPI pins
#define ETH_SPI_SCK     6
#define ETH_SPI_MISO    2
#define ETH_SPI_MOSI    7


#define LED_PIN         2
#define ANALOG_IN       3
#define TOP_THRESHHOLD    30
#define BOTTOM_THRESHHOLD 20
// Initialize Telegram BOT
#define BOT_TOKEN "XXXXXXXXXXXXXXXXXXXX"  // your Bot Token (Get from Botfather)
// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "XXXXXXX"
static bool eth_connected = false;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Checks for new messages every 1 second.
int gasPostDelay = 1000;
unsigned long lastTimeBotRan;
Adafruit_BMP280 bmp; // I2C

void onEvent(arduino_event_id_t event, arduino_event_info_t info)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-eth0");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      ETH.printTo(Serial);
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
    default:
      break;
  }
}


void readSensorDataAndPost(){
    float sensorValue=bmp.readTemperature();
    if(sensorValue>TOP_THRESHHOLD){
      Serial.println("Sensor value is above top threshold:"+String(sensorValue));
      bot.sendMessage(CHAT_ID, "Sensor value is above top threshold:"+String(sensorValue), "");
    }
    if(sensorValue<BOTTOM_THRESHHOLD){
      Serial.println("Sensor value is below bottom threshold(sensor is probably disconnected):"+String(sensorValue));
      bot.sendMessage(CHAT_ID, "Sensor value is below bottom threshold(sensor is probably disconnected):"+String(sensorValue), "");
    }
}

void setup() {
  Serial.begin(115200);

  Serial.println("ESP32-C6-Bug Sensor Reader\n");
  Wire.begin(21,20);
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1) delay(10);
  }
  WiFi.onEvent(onEvent);
  SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
  ETH.begin(ETH_TYPE, ETH_ADDR, ETH_CS, ETH_IRQ, ETH_RST, SPI);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  
}

void loop() {
  if(eth_connected){
    if (millis() > lastTimeBotRan + gasPostDelay)  {
      readSensorDataAndPost();
      lastTimeBotRan = millis();
    }
  }
  delay(1);
}

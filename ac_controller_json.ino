/*
 * AIR CONDITIONER CONTROLLER
 * Maxime MOREILLON
 * 
 * Air conditioner remote code decoding by Perhof:
 * https://perhof.wordpress.com/2015/03/29/reverse-engineering-hitachi-air-conditioner-infrared-remote-commands/
 * 
 * Board type: Wemos D1 Mini
 * 
 */


// Libraries
#include <ESP8266WiFi.h> // Main ESP8266 library
#include <ArduinoOTA.h> // OTA update library
#include <WiFiUdp.h> // Required for OTA
#include <PubSubClient.h>
#include <DHT.h> // Temperature and humidity sensor
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#include "credentials.h"

//#include "aircon_kitchen_nagoya.h";
#include "aircon_bedroom_nagoya.h";

// MQTT
#define MQTT_BROKER_ADDRESS IPAddress(192, 168, 1, 2)
#define MQTT_PORT 1883
#define MQTT_LAST_WILL "{'state':'disconnected'}"
#define MQTT_QOS 1
#define MQTT_RETAIN true

// Web server
#define WWW_PORT 80

// Pin mapping
#define DHT_PIN D1
#define PIR_PIN D2
#define IR_LED_PIN D6

// DHT
#define DHT_PUBLISH_PERIOD 300000 // [ms] = 5 minutes
#define DHT_READ_PERIOD 10000 // [ms] = 10 seconds

// IR
#define IR_SIGNAL_LENGTH 331

// Global variables
WiFiClient wifi_client;
PubSubClient MQTT_client(wifi_client);
ESP8266WebServer web_server(WWW_PORT);

// AC variables
const char* AC_state = "unknown";

// IR signals
int IR_signal_off[IR_SIGNAL_LENGTH] = {3064, -2928, 3060, -4300, 616, -468, 632, -452, 616, -468, 608, -472, 620, -1552, 608, -1564, 604, -1572, 580, -512, 632, -1536, 580, -1592, 612, -1560, 580, -1592, 600, -488, 628, -452, 604, -480, 608, -1564, 632, -448, 604, -484, 580, -504, 632, -452, 608, -1564, 604, -1568, 632, -1540, 580, -1588, 620, -464, 612, -476, 632, -452, 632, -452, 628, -1540, 628, -1544, 628, -1540, 600, -1572, 612, -476, 580, -512, 628, -456, 580, -508, 608, -1560, 584, -1592, 608, -1560, 636, -1536, 632, -452, 596, -1572, 592, -1584, 632, -452, 632, -1536, 580, -508, 608, -472, 596, -1576, 584, -504, 608, -476, 612, -472, 632, -452, 632, -1532, 584, -1588, 608, -1564, 608, -1564, 632, -448, 580, -504, 608, -476, 580, -508, 632, -1536, 584, -1592, 580, -1592, 580, -1596, 580, -508, 624, -456, 636, -448, 624, -460, 628, -1540, 600, -1572, 624, -1548, 600, -1568, 608, -476, 636, -448, 580, -512, 600, -484, 608, -1560, 584, -1588, 580, -1592, 600, -1576, 624, -39152, 3056, -2936, 3056, -4300, 612, -476, 604, -476, 580, -512, 604, -480, 580, -1592, 580, -1596, 600, -1572, 600, -488, 620, -1544, 580, -1592, 580, -1592, 580, -1588, 580, -508, 620, -464, 620, -464, 580, -1592, 604, -484, 608, -476, 608, -476, 580, -512, 604, -1568, 600, -1572, 580, -1592, 608, -1564, 580, -504, 580, -512, 608, -476, 608, -480, 604, -1564, 580, -1588, 612, -1560, 580, -1596, 632, -448, 580, -504, 608, -476, 608, -476, 600, -1572, 632, -1536, 580, -1592, 608, -1564, 612, -468, 580, -1588, 584, -1592, 596, -484, 580, -1592, 584, -504, 620, -464, 600, -1568, 580, -508, 632, -448, 580, -504, 580, -508, 632, -1540, 580, -1588, 608, -1564, 636, -1536, 620, -460, 580, -508, 576, -512, 608, -476, 608, -1564, 580, -1596, 608, -1564, 596, -1580, 580, -512, 608, -476, 608, -476, 608, -480, 632, -1532, 584, -1592, 580, -1588, 624, -1548, 580, -504, 608, -472, 608, -480, 592, -488, 580, -1596, 580, -1592, 580, -1592, 580, -1596, 580};
int IR_signal_heater_on[IR_SIGNAL_LENGTH] =  {3028, -2964, 3024, -4336, 580, -504, 580, -508, 580, -508, 576, -508, 580, -1588, 580, -1592, 580, -1592, 580, -512, 580, -1592, 580, -1592, 580, -1596, 580, -1592, 576, -504, 580, -504, 580, -504, 580, -1596, 580, -1592, 580, -1592, 580, -508, 580, -508, 576, -504, 580, -508, 580, -1592, 576, -1592, 580, -504, 580, -508, 576, -508, 576, -508, 576, -1592, 580, -1592, 580, -1592, 580, -1596, 576, -508, 576, -508, 580, -508, 576, -504, 580, -1588, 580, -1592, 580, -1588, 580, -1592, 580, -504, 580, -1592, 580, -1592, 580, -504, 576, -1592, 580, -504, 580, -508, 576, -1596, 580, -504, 580, -508, 580, -508, 576, -504, 580, -1588, 580, -1596, 576, -1592, 580, -1592, 580, -504, 580, -512, 576, -508, 576, -508, 580, -1588, 580, -1596, 576, -1592, 580, -1596, 580, -504, 576, -508, 580, -504, 580, -508, 580, -1596, 576, -1592, 580, -1592, 580, -1596, 576, -504, 580, -504, 580, -508, 580, -508, 576, -1592, 576, -1592, 580, -1592, 580, -1592, 580, -39200, 3024, -2968, 3024, -4340, 580, -508, 580, -508, 580, -504, 576, -508, 576, -1592, 580, -1592, 576, -1592, 580, -504, 580, -1596, 580, -1588, 580, -1596, 580, -1592, 580, -500, 580, -508, 580, -504, 580, -1588, 580, -1592, 580, -1596, 580, -504, 580, -508, 580, -504, 580, -504, 580, -1596, 580, -1592, 580, -504, 576, -508, 576, -508, 580, -504, 580, -1596, 580, -1592, 580, -1592, 576, -1592, 580, -504, 580, -508, 580, -504, 576, -508, 576, -1592, 580, -1592, 576, -1596, 580, -1596, 580, -508, 580, -1596, 580, -1588, 580, -508, 580, -1596, 580, -508, 580, -508, 580, -1592, 580, -504, 580, -504, 580, -504, 580, -504, 580, -1592, 580, -1592, 580, -1592, 580, -1596, 580, -504, 580, -504, 580, -508, 580, -508, 580, -1592, 576, -1592, 580, -1592, 580, -1596, 576, -508, 580, -504, 576, -504, 580, -504, 580, -1592, 580, -1592, 576, -1592, 580, -1592, 580, -508, 580, -504, 580, -508, 580, -504, 580, -1596, 580, -1592, 580, -1588, 580, -1596, 576};
int IR_signal_cooler_on[IR_SIGNAL_LENGTH] = {3080,-2912,3080,-4284,636,-448,636,-448,636,-452,632,-452,632,-1540,636,-1536,636,-1540,608,-484,632,-1536,632,-1544,632,-1540,624,-1548,636,-452,632,-452,632,-452,632,-1536,636,-1536,628,-1548,636,-452,636,-452,632,-452,632,-452,632,-1536,636,-1536,632,-448,632,-456,636,-448,632,-452,636,-1536,636,-1532,636,-1536,632,-1544,636,-452,632,-460,632,-452,628,-460,636,-1536,636,-1540,632,-1536,632,-1540,632,-1536,636,-448,632,-1536,636,-452,632,-452,632,-1540,636,-448,636,-1536,636,-448,636,-452,632,-452,636,-448,632,-1540,632,-1536,636,-1536,632,-1536,636,-452,632,-452,636,-448,636,-452,636,-1540,636,-1540,636,-1536,636,-1544,632,-452,636,-448,632,-452,636,-448,632,-1540,632,-1540,632,-1536,636,-1540,632,-452,632,-452,632,-456,636,-448,636,-1540,632,-1540,632,-1540,636,-1540,632,-39164,3080,-2916,3080,-4280,632,-452,632,-452,632,-456,636,-448,632,-1544,632,-1544,636,-1536,636,-452,632,-1536,636,-1536,636,-1536,636,-1536,636,-448,636,-448,636,-452,636,-1536,636,-1540,636,-1540,632,-448,636,-452,632,-448,636,-448,636,-1536,636,-1536,636,-452,632,-452,636,-452,632,-452,636,-1536,632,-1540,632,-1536,636,-1536,636,-448,632,-452,636,-448,632,-456,632,-1536,636,-1536,636,-1536,636,-1536,636,-1536,632,-452,636,-1536,636,-448,636,-452,632,-1536,636,-452,636,-1536,636,-448,632,-452,632,-452,636,-452,632,-1540,636,-1536,636,-1536,632,-1536,636,-456,636,-448,636,-452,632,-452,632,-1536,636,-1540,636,-1536,636,-1536,636,-448,632,-452,636,-452,632,-452,636,-1536,636,-1536,636,-1536,632,-1540,632,-456,632,-448,636,-456,632,-452,636,-1536,636,-1540,636,-1536,636,-1536,636};

// DHT variables
DHT dht(DHT_PIN, DHT22);

void setup() {

  // Mandatory initial delay
  delay(10);

  // Serial init
  Serial.begin(115200);
  Serial.println();
  Serial.println(); // Separate serial stream from initial gibberish
  Serial.println(F(__FILE__ " " __DATE__ " " __TIME__));

  // IO init
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  dht.begin();
  wifi_setup();
  MQTT_setup();
  OTA_setup();
  web_server_setup();
}

void loop() {
  ArduinoOTA.handle();
  MQTT_client.loop();
  web_server.handleClient();
  read_PIR();
  read_DHT();
  wifi_connection_manager();
  MQTT_connection_manager();
}

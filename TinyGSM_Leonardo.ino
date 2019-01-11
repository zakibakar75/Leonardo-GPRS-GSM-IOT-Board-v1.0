/**************************************************************
 *
 * For this example, you need to install PubSubClient library:
 *   https://github.com/knolleary/pubsubclient
 *   or from http://librarymanager/all#PubSubClient
 *
 * TinyGSM Getting Started guide:
 *   http://tiny.cc/tiny-gsm-readme
 *
 * For more MQTT examples, see PubSubClient library
 *
 **************************************************************
 * Use Mosquitto client tools to work with MQTT
 *   Ubuntu/Linux: sudo apt-get install mosquitto-clients
 *   Windows:      https://mosquitto.org/download/
 *
 * Subscribe for messages:
 *   mosquitto_sub -h test.mosquitto.org -t GsmClientTest/init -t GsmClientTest/ledStatus -q 1
 * Toggle led:
 *   mosquitto_pub -h test.mosquitto.org -t GsmClientTest/led -q 1 -m "toggle"
 *
 * You can use Node-RED for wiring together MQTT-enabled devices
 *   https://nodered.org/
 * Also, take a look at these additional Node-RED modules:
 *   node-red-contrib-blynk-ws
 *   node-red-dashboard
 *
 **************************************************************/

// Select your modem:
#define TINY_GSM_MODEM_SIM800
// #define TINY_GSM_MODEM_SIM808
// #define TINY_GSM_MODEM_SIM900
// #define TINY_GSM_MODEM_UBLOX
// #define TINY_GSM_MODEM_BG96
// #define TINY_GSM_MODEM_A6
// #define TINY_GSM_MODEM_A7
// #define TINY_GSM_MODEM_M590
// #define TINY_GSM_MODEM_ESP8266
// #define TINY_GSM_MODEM_XBEE

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

 // the number of the Relay pin
const int Relay1 = 2;
const int Relay2 = 3;
const int Relay3 = 4;
const int Relay4 = 5;


// or Software Serial on Uno, Nano
//#include <SoftwareSerial.h>
//SoftwareSerial SerialAT(2, 3); // RX, TX


// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "net";
const char user[] = "maxis";
const char pass[] = "wap";

// MQTT details
const char* broker = "broker_url_here";

const char* topicInit = "myleo/init";

const char* topicHeartBeat = "myleo/heartbeat";

const char* topicRelay = "myleo/relay/#";

const char* topicRelay1 = "myleo/relay/1";
const char* topicRelay1Status = "myleo/relay1Status";

const char* topicRelay2 = "myleo/relay/2";
const char* topicRelay2Status = "myleo/relay2Status";

const char* topicRelay3 = "myleo/relay/3";
const char* topicRelay3Status = "myleo/relay3Status";

const char* topicRelay4 = "myleo/relay/4";
const char* topicRelay4Status = "myleo/relay4Status";

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

#define LED_PIN 13
int relay1Status = LOW;
int relay2Status = LOW;
int relay3Status = LOW;
int relay4Status = LOW;

long lastReconnectAttempt = 0;
long lastSentHeartBeat = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);

  for(int i=2;i<6;i++){ // initialize the Relay pins status:
      pinMode(i,OUTPUT);
      digitalWrite(i,LOW);
  }

  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    while (true);
  }
  SerialMon.println(" OK");

  SerialMon.print("Connecting to ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" fail");
    while (true);
  }
  SerialMon.println(" OK");

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
  //boolean status = mqtt.connect("myleo");

  // Or, if you want to authenticate MQTT:
  boolean status = mqtt.connect("myleo", "username", "password");

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" OK");
  mqtt.publish(topicInit, "MyLeo GSM Relay Board started");
  mqtt.subscribe(topicRelay);
  return mqtt.connected();
}

void loop() {

  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  if (mqtt.connected()) {
    // Heartbeat every 5 minutes
    unsigned long lapsedTime = millis();
    if (lapsedTime - lastSentHeartBeat > 300000L) {
      lastSentHeartBeat = lapsedTime;
      mqtt.publish(topicHeartBeat, "Alive");
    }
    delay(100);
  }

  mqtt.loop();
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicRelay1) {
    relay1Status = !relay1Status;
    digitalWrite(Relay1, relay1Status);
    mqtt.publish(topicRelay1Status, relay1Status ? "1" : "0");
  }

  else if (String(topic) == topicRelay2) {
    relay2Status = !relay2Status;
    digitalWrite(Relay2, relay2Status);
    mqtt.publish(topicRelay2Status, relay2Status ? "1" : "0");
  }

  else if (String(topic) == topicRelay3) {
    relay3Status = !relay3Status;
    digitalWrite(Relay3, relay3Status);
    mqtt.publish(topicRelay3Status, relay3Status ? "1" : "0");
  }

  else if (String(topic) == topicRelay4) {
    relay4Status = !relay4Status;
    digitalWrite(Relay4, relay4Status);
    mqtt.publish(topicRelay4Status, relay4Status ? "1" : "0");
  }
}




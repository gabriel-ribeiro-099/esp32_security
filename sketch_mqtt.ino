#include <WiFi.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <set>

using namespace std;

const char* ssid = "imd0902";
const char* password = "imd0902iot";
const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = "andersoncfs";
const char* mqttPassword = "aio_xSbL70jccMG5nIpA8iwO24lHoSby";

WiFiClient espClient;
PubSubClient client(espClient);

BLEScan* pBLEScan;
set<String> detectedDevices;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
    BLEAddress address = advertisedDevice.getAddress();
    String addressString = String(address.toString().c_str());

    if (detectedDevices.count(addressString) == 0) {
      detectedDevices.insert(addressString);
      }
    }
};

const int PIN_TO_SENSOR = 19;
int pinStateCurrent = LOW;
int pinStatePrevious = LOW;

#define DOOR_SENSOR_PIN  23
int currentDoorState;
int previousDoorState;

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    String clientId = "ESP32 - Sensores";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      client.publish("andersoncfs/feeds/oldLady", "Iniciando Comunicação");
      client.subscribe("andersoncfs/feeds/oldLady");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_TO_SENSOR, INPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10);

  setup_wifi();
  client.setServer(mqttserver, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  previousDoorState = currentDoorState;
  currentDoorState = digitalRead(DOOR_SENSOR_PIN);
  pinStatePrevious = pinStateCurrent;
  pinStateCurrent = digitalRead(PIN_TO_SENSOR);
  int contadorReset = 0;

  if (currentDoorState == HIGH && previousDoorState == LOW) {
    if (detectedDevices.count("ef:ef:2e:19:dc:b5") == 0 && detectedDevices.count("c2:08:03:02:13:25") == 0) {
      Serial.print("ALERT! The door was open, unrecognized bluetooth detected \n");
    }

    if (pinStatePrevious == LOW && pinStateCurrent == HIGH) {
      if (detectedDevices.count("ef:ef:2e:19:dc:b5") == 0 && detectedDevices.count("c2:08:03:02:13:25") == 0) {
        Serial.print("ALERT! Motion and an unrecognized bluetooth detected \n");
        char* valor = "1";
        client.publish("andersoncfs/feeds/oldLady", valor);
      }
    }

    if (contadorReset == 50) {
      detectedDevices.clear();
      contadorReset = 0;
      BLEDevice::init("");
      pBLEScan = BLEDevice::getScan();
      pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
      pBLEScan->setActiveScan(true);
      pBLEScan->start(10);
    } else {
      contadorReset++;
      delay(200);
    }
  }
  delay(200);
}

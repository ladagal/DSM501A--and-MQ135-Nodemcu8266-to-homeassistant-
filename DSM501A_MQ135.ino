
#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <ESP8266WiFi.h>
#include "secrets.h"
// MQ-135 senzor plynů připojení potřebné knihovny
#include <MQ135.h>
// nastavení propojovacích pinů MQ-135
#define pinA A0
// inicializace senzoru z knihovny
MQ135 senzorMQ = MQ135(pinA);

#define DUST_SENSOR_DIGITAL_PIN_PM10 12 // DSM501 Pin 2 - ESP8266 GPIO12/D6
#define DUST_SENSOR_DIGITAL_PIN_PM25 14 // DSM501 Pin 4 - ESP8266 GPIO14/D5 


char ssid[] = SECRET_SSID; // Wifi SSID , zapsat do secrets.h
char pass[] = SECRET_PASS; // Heslo k Wifi zapsat do secrets.h
char mqtt_user[] = MQTT_USERNAME; // Jmeno do MQTT , zapsat do secrets.h
char mqtt_pass[] = MQTT_PASSWORD; //Heslo do MQTT , zapsat do secrets.h
const char broker[] = MQTT_BROKER; // Adresa MQTT brokeru , zapsat do secrets.h

int port = 1883;

const char topicPM10p[] = "tele/DSM501A/PM10_particle";
const char topicPM10c[] = "tele/DSM501A/PM10_concentration";
const char topicPM25p[] = "tele/DSM501A/PM25_particle";
const char topicPM25c[] = "tele/DSM501A/PM25_concentration";
const char topicMQ135[] = "tele/MQ135/PPM_concentration";


char uniqueid[32];

unsigned long lpo_PM10 = 0;
unsigned long lpo_PM25 = 0;
unsigned long lpo_MQ135 = 0;

unsigned long starttime;

unsigned long sampletime_ms = 30000;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

void setup() {

Serial.begin(9600);

sprintf(uniqueid,"" ,ESP.getChipId());
Serial.print("DeviceId:");
Serial.println(uniqueid);

pinMode(DUST_SENSOR_DIGITAL_PIN_PM10, INPUT);
pinMode(DUST_SENSOR_DIGITAL_PIN_PM25, INPUT);


Serial.print("Pokus o připojení k WPA SSID: ");
Serial.println(ssid);
Serial.print("Pripojuji se: ");
Serial.println(ssid);
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, pass);

while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
// failed, retry
}

Serial.println("Jste pripojeni k siti");
Serial.println();
Serial.print("Pokus o pripojeni k brokerovi MQTT: ");
Serial.println(broker);
mqttClient.setUsernamePassword(mqtt_user,mqtt_pass);
if (!mqttClient.connect(broker, port)) {
Serial.print("Pripojeni MQTT se nezdarilo! Kod chyby =");
Serial.println(mqttClient.connectError());
}

Serial.println("Jste pripojeni k brokerovi MQTT!");

Serial.println();

starttime = millis();

}

float getParticlemgm3(float r) {

float mgm3 = 0.001915 * pow(r, 2) + 0.09522 * r - 0.04884;

return mgm3 < 0.0 ? 0.0 : mgm3;

}

void sendMessage(const char *topic, float value) {

char newtopic[128];

sprintf(newtopic, topic, uniqueid);

mqttClient.beginMessage(newtopic);

mqttClient.println(value);

mqttClient.endMessage();

}

void loop() {

mqttClient.poll();

lpo_PM10 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM10, LOW);
lpo_PM25 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM25, LOW);

unsigned long duration = millis() - starttime;

if (duration > sampletime_ms) {

float ratio = (lpo_PM10 - duration + sampletime_ms) / (sampletime_ms * 10.0); // Procento celého čísla 0=>100
float concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // krivka

sendMessage(topicPM10p,getParticlemgm3(ratio));
sendMessage(topicPM10c, concentration);

ratio = (lpo_PM25 - duration + sampletime_ms) / (sampletime_ms * 10.0); // Procento celého čísla 0=>100
concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // krivka
sendMessage(topicPM25p, getParticlemgm3(ratio));
sendMessage(topicPM25c, concentration);

lpo_PM10 = 0;
lpo_PM25 = 0;

//mq135------------------------


  // načtení koncentrace plynů v ppm do proměnné
  
 float ppm = senzorMQ.getPPM();
  sendMessage(topicMQ135, ppm);
  Serial.print(ppm);
  ppm = 0;
  // pauza před dalším měřením
starttime = millis();  
}

//mq135------------------------------------------------

}
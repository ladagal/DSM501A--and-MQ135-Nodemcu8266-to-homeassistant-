
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

#include<string.h>
#define PM1PIN 14
#define PM25PIN 12
byte buff[2];
unsigned long durationPM1;
unsigned long durationPM25;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancyPM1 = 0;
unsigned long lowpulseoccupancyPM25 = 0;

char ssid[] = SECRET_SSID; // Wifi SSID , zapsat do secrets.h
char pass[] = SECRET_PASS; // Heslo k Wifi zapsat do secrets.h
char mqtt_user[] = MQTT_USERNAME; // Jmeno do MQTT , zapsat do secrets.h
char mqtt_pass[] = MQTT_PASSWORD; //Heslo do MQTT , zapsat do secrets.h
const char broker[] = MQTT_BROKER; // Adresa MQTT brokeru , zapsat do secrets.h

int port = 1883;

const char topicPM10c[] = "tele/DSM501A/PM10_concentration";
const char topicPM25c[] = "tele/DSM501A/PM25_concentration";
const char topicMQ135[] = "tele/MQ135/PPM_concentration";

char uniqueid[32];

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
int i=0;
void setup() {

Serial.begin(9600);
Serial.println("Starting please wait 30s");
pinMode(PM1PIN,INPUT);
pinMode(PM25PIN,INPUT);

sprintf(uniqueid,"" ,ESP.getChipId());
Serial.print("DeviceId:");
Serial.println(uniqueid);

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

void sendMessage(const char *topic, float value) {

char newtopic[128];

sprintf(newtopic, topic, uniqueid);
mqttClient.beginMessage(newtopic);
mqttClient.println(value);
mqttClient.endMessage();

}

float calculateConcentration(long lowpulseInMicroSeconds, long durationinSeconds){
  
  float ratio = (lowpulseInMicroSeconds/1000000.0)/30.0*100.0; //Calculate the ratio
  float concentration = 0.001915 * pow(ratio,2) + 0.09522 * ratio - 0.04884;//Calculate the mg/m3
  Serial.print("lowpulseoccupancy:");
  Serial.print(lowpulseInMicroSeconds);
  Serial.print("    ratio:");
  Serial.print(ratio);
  Serial.print("    Concentration:");
  Serial.println(concentration);
  return concentration;
}

void loop() {

mqttClient.poll();

  durationPM1 = pulseIn(PM1PIN, LOW);
  durationPM25 = pulseIn(PM25PIN, LOW);
  
  lowpulseoccupancyPM1 += durationPM1;
  lowpulseoccupancyPM25 += durationPM25;
  
  endtime = millis();
  if ((endtime-starttime) > sampletime_ms) //Only after 30s has passed we calcualte the ratio
  {
    /*
    ratio1 = (lowpulseoccupancy/1000000.0)/30.0*100.0; //Calculate the ratio
    Serial.print("ratio1: ");
    Serial.println(ratio1);
    
    concentration = 0.001915 * pow(ratio1,2) + 0.09522 * ratio1 - 0.04884;//Calculate the mg/m3
    */
    float conPM1 = calculateConcentration(lowpulseoccupancyPM1,30);
    float conPM25 = calculateConcentration(lowpulseoccupancyPM25,30);
    Serial.print("PM1 ");
    Serial.print(conPM1);
    Serial.print("  PM25 ");
    Serial.println(conPM25);
    sendMessage(topicPM10c, conPM1);
    sendMessage(topicPM25c, conPM25);
    lowpulseoccupancyPM1 = 0;
    lowpulseoccupancyPM25 = 0;
    
//mq135------------------------------------------------
  float ppm = senzorMQ.getPPM();
      Serial.print("  PPM ");
    Serial.println(ppm);
  sendMessage(topicMQ135, ppm);

  ppm = 0;
  // pauza před dalším měřením
starttime = millis();  
  } 
}

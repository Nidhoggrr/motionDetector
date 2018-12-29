#include <Basecamp.hpp>
#include <Esp.h>
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

TimerHandle_t mqttWatchdog;
TimerHandle_t sendStatusTimer;
bool debug = false;
Basecamp iot;
static const int SensorPin1 = 34;
static const int SensorPin2 = 35;
int SensorValue1 = 0;
int SensorValue2 = 0;
int pirState = LOW;
int ledPin = 22;
String mqttTopic = "homie/empty";
String mqttTopicWD = "homie/empty/watchdog";
String mqttMessage = "false";

void onMqttConnect(bool sessionPresent) {
  iot.mqtt.subscribe((mqttTopicWD + "/set").c_str(), 2);;
  iot.mqtt.publish(("homie/" + iot.hostname + "/$homie").c_str(), 1, true, "3.0" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$name").c_str(), 1, true, "Bewegungsmelder Bett" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$state").c_str(), 1, true, "init" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$localip").c_str(), 1, true, iot.wifi.getIP().toString().c_str() );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$mac").c_str(), 1, true, iot.wifi.getHardwareMacAddress().c_str() );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$fw/name").c_str(), 1, true, "adke.movement" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$fw/version").c_str(), 1, true, "0.4" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$implementation").c_str(), 1, true, "esp32" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$stats").c_str(), 1, true, "" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$stats/interval").c_str(), 1, true, "120" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$stats/uptime").c_str(), 1, true, String(millis()).c_str() );
  iot.mqtt.publish(("homie/" + iot.hostname + "/$nodes").c_str(), 1, true, "status" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/$name").c_str(), 1, true, "Bewegung" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/$properties").c_str(), 1, true, "detected" );

  iot.mqtt.publish(("homie/" + iot.hostname + "/status/detected/$datatype").c_str(), 1, true, "boolean" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/detected").c_str(), 1, true, "false" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/detected/$name").c_str(), 1, true, "Bewegung erkannt" );
  
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/watchdog/$datatype").c_str(), 1, true, "string" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/watchdog").c_str(), 1, true, "0" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/watchdog/$settable").c_str(), 1, true, "true" );
  iot.mqtt.publish(("homie/" + iot.hostname + "/status/watchdog/$name").c_str(), 1, true, "Watchdog Tick" );

  iot.mqtt.publish(("homie/" + iot.hostname + "/$state").c_str(), 1, true, "ready" );
  iot.mqtt.setWill(("homie/" + iot.hostname + "/$state").c_str(), 1, true, "lost" );
  
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  xTimerReset(mqttWatchdog, 0);
}

void transmitStatus(bool sessionPresent) {
  iot.mqtt.publish(mqttTopic.c_str(), 1, true, mqttMessage.c_str());
  iot.mqtt.publish(("homie/" + iot.hostname + "/$stats/uptime").c_str(), 1, true, String(millis()).c_str() );
}

void reset() {
  ESP.restart();  
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(SensorPin1, INPUT);
  pinMode(SensorPin2, INPUT);
  for (int i=0; i <= 9; i++){
    delay(500);
    digitalWrite(ledPin, i%2);
  }
  iot.begin();
  sendStatusTimer = xTimerCreate("statusTime", pdMS_TO_TICKS(120000), pdTRUE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(transmitStatus));
  mqttWatchdog = xTimerCreate("WD", pdMS_TO_TICKS(122000), pdTRUE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(reset));
  mqttTopicWD = "homie/" + iot.hostname + "/status/watchdog";
  mqttTopic="homie/" + iot.hostname + "/status/detected";
  iot.mqtt.onConnect(onMqttConnect);
  iot.mqtt.connect();
  xTimerStart(mqttWatchdog, 0);
  xTimerStart(sendStatusTimer, 0);
}

void loop() {
  SensorValue1 = digitalRead(SensorPin1);
  SensorValue2 = digitalRead(SensorPin2);
  if (SensorValue1 == HIGH || SensorValue2 == HIGH) {
    digitalWrite(ledPin, LOW);
    if (pirState == LOW) {
      if (debug) Serial.println("Motion detected! Sensor1: " + String(SensorValue1) + " Sensor2: " + String(SensorValue2));
      mqttMessage = "true";
      transmitStatus(true);
      pirState = HIGH;
    }
  } else {
    digitalWrite(ledPin, HIGH);
    if (pirState == HIGH) {
      if (debug) Serial.println("Motion ended!");
      mqttMessage = "false";
      transmitStatus(true);
      pirState = LOW;
    }
  }
}

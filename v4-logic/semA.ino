/*********************************************************
  SEMÁFORO A (MAESTRO)
**********************************************************/
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_ROJO 2
#define LED_VERDE 3

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "iot.ac.uma.es";
const char* mqtt_user = "IOT2";
const char* mqtt_pass = "FGQQJUg3";

const unsigned long MIN_GREEN_A = 30000;
const unsigned long MAX_GREEN_B = 10000;
const unsigned long NO_CAR_B_TIMEOUT = 4000;

WiFiClient wClient;
PubSubClient mqtt_client(wClient);

String topic_PUBLICACION = "IOT2/semaforo/A/publica";
String topic_CAR_A = "IOT2/semaforo/control/carA";
String topic_CAR_B = "IOT2/semaforo/control/carB";

bool verdeA = true;
bool carAtA = false;
bool carAtB = false;

unsigned long greenAStart = 0;
unsigned long greenBStart = 0;
unsigned long bStateChangeTime = 0;

//-----------------------------------------------------
void procesa_mensaje(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("TOPIC: ");
  Serial.print(topic);
  Serial.print(" | MSG: ");
  Serial.println(msg);

  if (String(topic) == topic_CAR_A) {
    carAtA = (msg == "CAR");
  }

  if (String(topic) == topic_CAR_B) {
    if (msg == "CAR") {
      carAtB = true;
    } else {
      carAtB = false;
    }
    bStateChangeTime = millis();
  }
}

//-----------------------------------------------------

void publicaEstado(const char* reason = nullptr) {
  const char* estado = verdeA ? "VERDE" : "ROJO";
  mqtt_client.publish(topic_PUBLICACION.c_str(), estado, true); // retain
  Serial.print("[PUB] A/publica=");
  Serial.print(estado);
  if (reason) { Serial.print(" ("); Serial.print(reason); Serial.print(")"); }
  Serial.println();
}

void ensure_mqtt() {
  while (!mqtt_client.connected()) {
    Serial.print("MQTT connect...");
    if (mqtt_client.connect("SemaforoA_ESP32", mqtt_user, mqtt_pass)) {
      Serial.println("ok");
      mqtt_client.subscribe("IOT2/semaforo/control/#");  // wildcard
      publicaEstado("reconnect");
    } else {
      Serial.print("fail rc="); Serial.println(mqtt_client.state());
      delay(200);   // <-- keep short
      yield();      // <-- let Wi-Fi/MQTT breathe
    }
  }
}

//-----------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setKeepAlive(60);
  mqtt_client.setSocketTimeout(6);
  mqtt_client.setCallback(procesa_mensaje);

  while (!mqtt_client.connected()) {
    mqtt_client.connect("SemaforoA_ESP32", mqtt_user, mqtt_pass);
    delay(500);
  }

  mqtt_client.subscribe(topic_CAR_A.c_str());
  mqtt_client.subscribe(topic_CAR_B.c_str());

  greenAStart = millis();
  publicaEstado();
  bStateChangeTime = millis();
}

//-----------------------------------------------------
void loop() {
  if (!mqtt_client.connected()) mqtt_client.connect("SemaforoA_ESP32", mqtt_user, mqtt_pass);
  mqtt_client.loop();

  unsigned long now = millis();

  if (verdeA) {
    if (carAtB && now - greenAStart >= MIN_GREEN_A) {
      verdeA = false;
      greenBStart = now;
      publicaEstado();
    }
  } else {
    
    if (!carAtB && !carAtA && 
        (now - bStateChangeTime >= NO_CAR_B_TIMEOUT)) {
      verdeA = true;
      greenAStart = now;
      publicaEstado("B=CLEAR timeout, sin coches en ningún lado -> A=VERDE");
    }

    else if ((now - greenBStart >= MAX_GREEN_B) && carAtA) {
      verdeA = true;
      greenAStart = now;
      publicaEstado("MAX_GREEN_B & A=CAR");
    }

    else if (!carAtB && 
        (now - bStateChangeTime >= NO_CAR_B_TIMEOUT) &&
        (now - greenBStart <= MAX_GREEN_B)) {
      verdeA = true;
      greenAStart = now;
      publicaEstado("B=CLEAR timeout dentro de MAX");
    }
  }

  digitalWrite(LED_VERDE, verdeA);
  digitalWrite(LED_ROJO, !verdeA);

  static unsigned long lastPrint = 0;
  if (now - lastPrint > 2000) {
    Serial.print("A=");
    Serial.print(verdeA ? "VERDE" : "ROJO");
    Serial.print(" | carA=");
    Serial.print(carAtA);
    Serial.print(" | carB=");
    Serial.println(carAtB);
    lastPrint = now;
  }
  delay(1);

}

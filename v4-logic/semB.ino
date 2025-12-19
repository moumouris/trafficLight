/*********************************************************
  SEMÁFORO B (sin sensor)
  Esclavo: sigue el estado publicado por A
**********************************************************/
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_ROJO 7
#define LED_VERDE 6

const String ssid = "Wokwi-GUEST";
const String password = "";
const String mqtt_server = "iot.ac.uma.es";
const String mqtt_user = "IOT2";
const String mqtt_pass = "FGQQJUg3";

WiFiClient wClient;
PubSubClient mqtt_client(wClient);

String ID_PLACA = "SemaforoB_ESP32";
String topic_PUBLICACION = "IOT2/semaforo/B/publica";
String topic_SUSCRIPCION = "IOT2/semaforo/A/publica";

bool verdeB = false;  // B inicia en rojo

//-----------------------------------------------------
void conecta_wifi() {
  Serial.println("Conectando a WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str(), 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
}

//-----------------------------------------------------
void conecta_mqtt() {
  while (!mqtt_client.connected()) {
    Serial.print("Conectando a broker MQTT...");
    if (mqtt_client.connect(ID_PLACA.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
      Serial.println(" conectado!");
      mqtt_client.subscribe(topic_SUSCRIPCION.c_str());
    } else {
      Serial.println(" fallo, reintento en 5s");
      delay(5000);
    }
  }
}

//-----------------------------------------------------
void procesa_mensaje(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("Mensaje recibido de A: " + msg);

  if (msg == "VERDE") verdeB = false; // A está verde → B rojo
  if (msg == "ROJO")  verdeB = true;  // A está rojo → B verde

  digitalWrite(LED_VERDE, verdeB);
  digitalWrite(LED_ROJO, !verdeB);
  mqtt_client.publish(topic_PUBLICACION.c_str(), verdeB ? "VERDE" : "ROJO");
}

//-----------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  conecta_wifi();
  mqtt_client.setServer(mqtt_server.c_str(), 1883);
  mqtt_client.setCallback(procesa_mensaje);
  conecta_mqtt();
  mqtt_client.publish(topic_PUBLICACION.c_str(),
                    verdeB ? "VERDE" : "ROJO",
                    true);   // <-- RETAIN

}

//-----------------------------------------------------
void loop() {
  if (!mqtt_client.connected()) conecta_mqtt();
  mqtt_client.loop();
  delay(100);
}

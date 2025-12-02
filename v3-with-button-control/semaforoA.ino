/*********************************************************
  SEMÁFORO A (con sensor ultrasónico)
  Maestro: controla y publica estado a B
**********************************************************/
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_ROJO 2
#define LED_VERDE 3
#define TRIG_PIN 5
#define ECHO_PIN 4

const String ssid = "Wokwi-GUEST";
const String password = "";
const String mqtt_server = "iot.ac.uma.es";
const String mqtt_user = "IOT2";
const String mqtt_pass = "FGQQJUg3";

WiFiClient wClient;
PubSubClient mqtt_client(wClient);

String ID_PLACA = "SemaforoA";
String topic_PUBLICACION = "IOT2/semaforo/A/publica";
String topic_SUSCRIPCION = "IOT2/semaforo/B/publica";
String topic_SUSCRIPCION_CONTROL = "IOT2/control/publica";

bool verdeA = true;  // A inicia en verde
unsigned long tiempoCambio = 0;
const unsigned long intervalo = 5000;

void procesa_mensaje(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("Mensaje recibido de " + String(topic) + ": " + msg);

  if (String(topic) == topic_SUSCRIPCION_CONTROL) {
    if (msg == "VERDE") verdeA = true; 
    if (msg == "ROJO")  verdeA = false; 
    Serial.print(String(verdeA));
    digitalWrite(LED_VERDE, verdeA);
    digitalWrite(LED_ROJO, !verdeA);
    publicaEstado();
  }

}

//-----------------------------------------------------
void conecta_wifi() {
  Serial.println("Conectando a WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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
      mqtt_client.subscribe(topic_SUSCRIPCION_CONTROL.c_str());
    } else {
      Serial.println(" fallo, reintento en 5s");
      delay(5000);
    }
  }
}

//-----------------------------------------------------
int medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracion = pulseIn(ECHO_PIN, HIGH);
  return duracion * 0.034 / 2;
}

//-----------------------------------------------------
void publicaEstado() {
  String estado = verdeA ? "VERDE" : "ROJO";
  mqtt_client.publish(topic_PUBLICACION.c_str(), estado.c_str());
  Serial.println("Publicado: " + estado);
}

//-----------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  conecta_wifi();
  mqtt_client.setServer(mqtt_server.c_str(), 1883);
  mqtt_client.setCallback(procesa_mensaje);
  conecta_mqtt();
  publicaEstado();
}

//-----------------------------------------------------
void loop() {
  if (!mqtt_client.connected()) conecta_mqtt();
  mqtt_client.loop();

  int dist = medirDistancia();
  unsigned long ahora = millis();

  if (dist > 0 && dist < 50) {
    // Detecta peatón → A se pone verde
    verdeA = true;
    tiempoCambio = ahora;
    publicaEstado();
  } else if (ahora - tiempoCambio > intervalo) {
    // Alterna si no hay peatón
    verdeA = !verdeA;
    tiempoCambio = ahora;
    publicaEstado();
  }

  digitalWrite(LED_VERDE, verdeA);
  digitalWrite(LED_ROJO, !verdeA);

  delay(200);
}

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <BH1750.h>

#define DHTTYPE DHT11 // DHT 11
#define dht_dpin 0
DHT dht(dht_dpin, DHTTYPE);

// Configuración del sensor BH1750
BH1750 lightMeter;

#include "secrets.h"

// Conexión a Wifi
const char ssid[] = "Nico's iPhone";
const char pass[] = "umbs3225";

// Usuario uniandes sin @uniandes.edu.co
#define HOSTNAME "n.carvajalc"

// Conexión a Mosquitto
const char MQTT_HOST[] = "iotlab.virtual.uniandes.edu.co";
const int MQTT_PORT = 8082;
const char MQTT_USER[] = "n.carvajalc";
const char MQTT_PASS[] = "201922019";
const char MQTT_SUB_TOPIC[] = HOSTNAME "/";
const char MQTT_PUB_TOPIC1[] = "humedad/bogota/" HOSTNAME;
const char MQTT_PUB_TOPIC2[] = "temperatura/bogota/" HOSTNAME;
const char MQTT_PUB_TOPIC3[] = "luminosidad/bogota/" HOSTNAME;

BearSSL::WiFiClientSecure net;
PubSubClient client(net);

time_t now;
unsigned long lastMillis = 0;

void mqtt_connect()
{
  while (!client.connected())
  {
    Serial.print("Time: ");
    Serial.print(ctime(&now));
    Serial.print("MQTT connecting ... ");
    if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
    {
      Serial.println("connected.");
    }
    else
    {
      Serial.println("Problema con la conexión, revise los valores de las constantes MQTT");
      Serial.print("Código de error = ");
      Serial.println(client.state());
      if (client.state() == MQTT_CONNECT_UNAUTHORIZED)
      {
        ESP.deepSleep(0);
      }
      delay(5000);
    }
  }
}

void receivedCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    if (WiFi.status() == WL_NO_SSID_AVAIL || WiFi.status() == WL_WRONG_PASSWORD)
    {
      Serial.print("\nProblema con la conexión, revise los valores de las constantes ssid y pass");
      ESP.deepSleep(0);
    }
    else if (WiFi.status() == WL_CONNECT_FAILED)
    {
      Serial.print("\nNo se ha logrado conectar con la red, resetee el node y vuelva a intentar");
      ESP.deepSleep(0);
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println("connected!");

  Serial.print("Setting time using SNTP");
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < 1510592825)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  net.setInsecure();

  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(receivedCallback);
  mqtt_connect();

  // Iniciar el sensor DHT11
  dht.begin();

  // Iniciar el sensor BH1750
  Wire.begin(D2, D1); // Configurar los pines SDA y SCL para NodeMCU
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println(F("Sensor BH1750 iniciado correctamente."));
  }
  else
  {
    Serial.println(F("Error iniciando el sensor BH1750."));
    while (1)
      ;
  }
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Checking wifi");
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(10);
    }
    Serial.println("connected");
  }
  else
  {
    if (!client.connected())
    {
      mqtt_connect();
    }
    else
    {
      client.loop();
    }
  }

  now = time(nullptr);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float lux = lightMeter.readLightLevel();

  String json;

  // JSON para humedad
  if (!isnan(h))
  {
    json = "{\"value\": " + String(h) + "}";
    char payload1[json.length() + 1];
    json.toCharArray(payload1, json.length() + 1);
    client.publish(MQTT_PUB_TOPIC1, payload1, false);
    Serial.print(MQTT_PUB_TOPIC1);
    Serial.print(" -> ");
    Serial.println(payload1);
  }

  // JSON para temperatura
  if (!isnan(t))
  {
    json = "{\"value\": " + String(t) + "}";
    char payload2[json.length() + 1];
    json.toCharArray(payload2, json.length() + 1);
    client.publish(MQTT_PUB_TOPIC2, payload2, false);
    Serial.print(MQTT_PUB_TOPIC2);
    Serial.print(" -> ");
    Serial.println(payload2);
  }

  // JSON para luz
  if (lux >= 0)
  {
    json = "{\"value\": " + String(lux) + "}";
    char payload3[json.length() + 1];
    json.toCharArray(payload3, json.length() + 1);
    client.publish(MQTT_PUB_TOPIC3, payload3, false);
    Serial.print(MQTT_PUB_TOPIC3);
    Serial.print(" -> ");
    Serial.println(payload3);
  }
  else
  {
    Serial.println("Error leyendo la luz");
  }

  delay(5000);
}
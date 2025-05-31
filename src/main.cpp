
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Servo.h>

#define SSID_NAME "."                         // Your Wifi Network name
#define SSID_PASSWORD "."                     // Your Wifi network password
#define MQTT_BROKER "smartnest.cz"            // Broker host
#define MQTT_PORT 1883                        // Broker port
#define MQTT_USERNAME "nipets"                // Username from Smartnest
#define MQTT_PASSWORD "."                     // Password from Smartnest (or API key)
#define MQTT_CLIENT "."                       // Device Id from smartnest
#define FIRMWARE_VERSION "Acceso interno IAU" // Custom name for this program

WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;     // Instancia del servo
int servoPin = D1; // Puedes usar el pin que gustes compatible con PWM

void startWifi();
void startMqtt();
void checkMqtt();
int splitTopic(char *topic, char *tokens[], int tokensNumber);
void callback(char *topic, byte *payload, unsigned int length);
void sendToBroker(char *topic, char *message);

void setup()
{
  myServo.attach(servoPin); // Inicializa el servo
  myServo.write(0);         // Asegúrate de que esté en posición 0 al inicio
  Serial.begin(115200);
  startWifi();
  startMqtt();
}

void loop()
{
  client.loop();
  checkMqtt();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Topic:");
  Serial.println(topic);
  int tokensNumber = 10;
  char *tokens[tokensNumber];
  char message[length + 1];
  splitTopic(topic, tokens, tokensNumber);
  for (unsigned int i = 0; i < length; i++)
  {
    message[i] = (char)payload[i];
  }
  message[length] = '\0'; // Cierra el string
  Serial.print("Message:");
  Serial.println(message);

  //------------------ACTIONS HERE---------------------------------
  if (strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "powerState") == 0)
  {
    if (strcmp(message, "ON") == 0)
    {
      // Mover servo a 180 grados
      myServo.write(180);
      delay(1000); // Esperar 1 segundo

      // Regresar a 0 grados
      myServo.write(0);
      delay(1000); // Esperar para completar el movimiento antes de enviar el OFF

      // Publicar que el dispositivo debe apagarse
      sendToBroker("report/powerState", "OFF");
    }
    else if (strcmp(message, "OFF") == 0)
    {
      // Opcionalmente podrías detener el servo o moverlo a una posición de reposo
      myServo.write(0);
    }
  }
}

void startWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);
  Serial.println("Connecting ...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10)
  {
    attempts++;
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println('\n');
    Serial.println("I could not connect to the wifi network after 10 attempts \n");
  }

  delay(500);
}

void startMqtt()
{
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");

    if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("connected");
    }
    else
    {
      if (client.state() == 5)
      {
        Serial.println("Connection not allowed by broker, possible reasons:");
        Serial.println("- Device is already online. Wait some seconds until it appears offline for the broker");
        Serial.println("- Wrong Username or password. Check credentials");
        Serial.println("- Client Id does not belong to this username, verify ClientId");
      }
      else
      {
        Serial.println("Not possible to connect to Broker Error code:");
        Serial.print(client.state());
      }
      delay(3000);
    }
  }

  char subscribeTopic[100];
  sprintf(subscribeTopic, "%s/#", MQTT_CLIENT);
  client.subscribe(subscribeTopic);

  sendToBroker("report/online", "true");
  delay(100);
  sendToBroker("report/firmware", FIRMWARE_VERSION);
  delay(100);
  sendToBroker("report/ip", (char *)WiFi.localIP().toString().c_str());
  delay(100);
  sendToBroker("report/network", (char *)WiFi.SSID().c_str());
  delay(100);

  char signal[5];
  sprintf(signal, "%d", WiFi.RSSI());
  sendToBroker("report/signal", signal);
  delay(100);
}

int splitTopic(char *topic, char *tokens[], int tokensNumber)
{
  const char s[2] = "/";
  int pos = 0;
  tokens[0] = strtok(topic, s);

  while (pos < tokensNumber - 1 && tokens[pos] != NULL)
  {
    pos++;
    tokens[pos] = strtok(NULL, s);
  }

  return pos;
}

void checkMqtt()
{
  if (!client.connected())
  {
    startMqtt();
  }
}

void sendToBroker(char *topic, char *message)
{
  if (client.connected())
  {
    char topicArr[100];
    sprintf(topicArr, "%s/%s", MQTT_CLIENT, topic);
    client.publish(topicArr, message);
  }
}

#include <ESP8266WiFi.h>
#include <Servo.h>

const char *ssid = "******";     // Reemplaza con tu SSID
const char *password = "******"; // Reemplaza con tu contraseña

WiFiServer server(80);
Servo myServo;

unsigned long lastMoveTime = 0;
bool moving = false;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(100);
  Serial.println(WiFi.localIP());

  server.begin();
  myServo.attach(D1);
  myServo.write(0); // posición inicial
}

void loop()
{
  WiFiClient client = server.available();
  if (client)
  {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /mover") != -1)
    {
      if (!moving)
      {
        myServo.write(90);
        lastMoveTime = millis();
        moving = true;
      }
    }

    // Respuesta HTML simple (sin retrasos)
    client.print(F(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<!DOCTYPE html><html><body>"
        "<h1>ESP8266 Servo</h1>"
        "<p>Refresca la página para mover el motor</p>"
        "</body></html>"));
    client.stop();
  }

  // Lógica no bloqueante para volver a 0°
  if (moving && millis() - lastMoveTime > 500)
  {
    myServo.write(0);
    moving = false;
  }
}
#include <Arduino.h>

#include <Wire.h>
#include <BH1750.h>

// Crear un objeto BH1750
BH1750 lightMeter;

void setup()
{
  // Iniciar la comunicación serie
  Serial.begin(9600);

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
  // Leer el valor de lux del sensor
  float lux = lightMeter.readLightLevel();
  if (lux < 0)
  {
    Serial.println("Error leyendo la luz");
  }
  else
  {
    // Imprimir la lectura en el monitor serial
    Serial.print("Nivel de luz: ");
    Serial.print(lux);
    Serial.println(" lx");
  }

  // Esperar 1 segundo antes de leer nuevamente
  delay(1000);
}
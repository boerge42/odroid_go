

#include <odroid_go.h>
#include "sensors/Wire.h"
#include "sensors/Adafruit_BME280.h"

Adafruit_BME280 bme(0x77);


const int PIN_I2C_SDA = 15;
const int PIN_I2C_SCL = 4;

void setup() {
  GO.begin();
  GO.lcd.clearDisplay();
  GO.lcd.setTextSize(2);
  GO.lcd.setTextFont(1);
  GO.lcd.setTextColor(RED);
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("BME280:");

  
  GO.lcd.setCursor(0, 200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  GO.lcd.println(bme.begin());



  
}

void loop() {
  GO.lcd.setTextColor(WHITE);
  GO.lcd.setCursor(0, 40);
  GO.lcd.print("Temperature: ");
  GO.lcd.print(bme.readTemperature());
  GO.lcd.println(" *C");

  
  GO.lcd.print("Pressure...: ");
  GO.lcd.print(1023);
  GO.lcd.println(" hPa");

  GO.lcd.print("Humidity...: ");
  GO.lcd.print(42.9);
  GO.lcd.println(" %");

  GO.lcd.print("Altitude...: ");
  GO.lcd.print(42);
  GO.lcd.println(" m");

  //  BME280Temperature = bme.readTemperature();
  //  BME280Pressure = (bme.readPressure() / 100);
  //  BME280Humidity = bme.readHumidity();
  //  BME280Altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);


  delay(1000);
}

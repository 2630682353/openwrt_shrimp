#include "HX711.h"
//HX711 Arduino Library by Bogdan
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 5;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(198.055786);    //只需要改这个值 this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();	                  
}

void loop() {
  
  
  if (scale.is_ready()) {
    float reading = scale.get_units(10);
    Serial.print("HX711 reading: ");
    Serial.println(reading);
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000);
  
}

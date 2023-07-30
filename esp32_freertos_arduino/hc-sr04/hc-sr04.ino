/*
  HC-SR04 Ultrasonic Sensor Example.

  Turn the LED on when an object is within 100cm range.

  Copyright (C) 2021, Uri Shaked
*/

#define ECHO_PIN 5
#define TRIG_PIN 4

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);//清除trigPin条件
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);//设置trigPin HIGH (ACTIVE)为10微秒
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  int duration = pulseIn(ECHO_PIN, HIGH);//读取ECHO_PIN，返回以微秒为单位的声波传播时间
  return duration * 0.034 / 2;//声波速度除以2(来回)
}

void loop() {
  float distance = readDistanceCM();

  Serial.print("Measured distance: ");
  Serial.println(distance);

  delay(100);
}

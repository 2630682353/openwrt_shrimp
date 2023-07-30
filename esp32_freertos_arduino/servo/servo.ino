#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32
int pos = 0;  // variable to store the servo position
int servoPin = 4;
void setup() {
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);  // standard 50 hz servo
  myservo.attach(servoPin, 500, 2500);
  myservo.write(pos);
  delay(1000);
}
void loop() {
  //电压一定要为5v，不然带不动
  myservo.write(50); //180角度时要持续输出pwm脉冲，不稳定
  delay(2000);
  myservo.write(0);
  delay(6000);  // tell servo to go to position in variable 'pos' 0角度时稳定
   
}

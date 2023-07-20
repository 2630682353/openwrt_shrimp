#include <Deneyap_Servo.h>
Servo myservo;  // 定义Servo对象来控制
int pos = 0;
void setup() {
  myservo.attach(4);  // 控制线（橙色）连接数字引脚9接受PWM信号
}
void loop() {
      myservo.write(90);
      // 舵机角度写入
     
      delay(1000);
      // 从90°到0°    
      myservo.write(180);              
      // 舵机角度写入    
      delay(1000);
}

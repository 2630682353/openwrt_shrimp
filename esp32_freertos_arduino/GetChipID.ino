#include <WiFi.h>

const char* ssid     = "zc_test";
const char* password = "58285390";

const char* host = "192.168.10.103";
const char* streamId   = "....................";
const char* privateKey = "....................";

void main_task( void * parameter )
{
    vTaskDelay(10);
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    while(1)
    {
      // Use WiFiClient class to create TCP connections
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
          Serial.println("connection failed");
          return;
      }

      // We now create a URI for the request
      String url = "/portal_cgi?opt=query_temper&client_mac=all&period=recent&client_temper_index=all";
      Serial.print("Requesting URL: ");
      Serial.println(url);
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Connection: close\r\n\r\n");
      unsigned long timeout = millis();
      while (client.available() == 0) {
          if (millis() - timeout > 5000) {
              Serial.println(">>> Client Timeout !");
              client.stop();
              return;
          }
      }

      // Read all the lines of the reply from server and print them to Serial
      while(client.available()) {
          String line = client.readStringUntil('\r');
          Serial.print(line);
      }

      Serial.println();
      Serial.println("closing connection");
      vTaskDelay(5000);
    }
}
 
  
 
void task2( void * parameter)
{
    char out_str[100] = {0};
    for( int i = 0;i<100;i++ ){
        sprintf(out_str, "Hello from task 2 %d", i);
        Serial.println(out_str);
        vTaskDelay(10000); 
    }
}
 
void setup() {
  Serial.begin(115200);
  
  xTaskCreate(
                    main_task,          //指定任务函数，也就是上面那个task1函数
                    "TaskMain",        //任务名称
                    10000,            //任务堆栈大小
                    NULL,             //作为任务输入传递的参数
                    1,                //优先级
                    NULL);            //任务句柄
 
  xTaskCreate(
                    task2,         
                    "TaskTwo",        
                    10000,            
                    NULL,             
                    1,               
                    NULL); 
}
 
 
void loop() {

}

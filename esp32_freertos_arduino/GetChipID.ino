#include <dummy.h>

#include <AsyncUDP.h>
#include "list.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>

#define PIN_NUM 40
#define HEART_BEAT_INTERVAL 10
const char* ssid     = "zc_test";
const char* password = "58285390";
String dev_mac = "aa:aa:aa:aa:aa:aa";

QueueHandle_t msg_queue;
LIST_HEAD(my_timer_list);

enum SENSOR_TYPE
{
	SENSOR_TEMPER = 1,
	SENSOR_FEED,
	SENSOR_AIR_PRESSURE,
	SENSOR_WATER_LEVEL,
	SENSOR_HEART_BEAT = 100,
};
typedef int (*timer_func)(void *param);

typedef struct sensor_info_st
{
	int report_interval;
	int id;
	unsigned char type;
	unsigned char pin;
	unsigned char pool_id;
}sensor_info_t;

typedef struct msg_item_st
{
	int msg_type;
}msg_item_t;


typedef struct util_timer_st {
	unsigned long expire;
	int (*cb_func)(void *para);
	int loop;
	int id;
	int pin;
	int interval;
	int timer_type;
	int pool_id;
	char other_param[32];
  char *para;
	struct list_head list;
}util_timer;

static int timer_timeslot;
static pthread_mutex_t timer_mutex;

void timer_handler() {
	unsigned long cur = millis();
	util_timer *p = NULL;
	util_timer *n = NULL;
	list_for_each_entry_safe(p, n, &my_timer_list, list) {
		if (cur >= p->expire) {
			p->cb_func(p->para);
			if (!p->loop) {
				list_del(&p->list);
				if (p->para)
					free(p->para);
				free(p);
			}
			else {
				p->expire = cur + p->interval*1000;
			}
		}
	}
}

int del_timer(int type)
{
	util_timer *p = NULL;
	util_timer *n = NULL;
	list_for_each_entry_safe(p, n, &my_timer_list, list) {
		if (p->timer_type == type) {
			list_del(&p->list);
			if (p->para)
				free(p->para);
			free(p);
		}			
	}
	return 0;
}

util_timer* find_timer(int pin)
{
	util_timer *t = NULL;
	list_for_each_entry(t, &my_timer_list, list) {
		if (t->pin == pin) {
			return t;
		}
	}
	return NULL;
}

int sensor_temper_func(void *param)
{
  int time = millis();
  Serial.print(time);
	Serial.println("int temper func ");
}
int sensor_heart_beat(void *param)
{
	String url = "/portal_cgi?opt=heart_beat&client_mac="+dev_mac;
	DynamicJsonDocument json_obj(1024);
	String out;
	http_send(url, out);
  deserializeJson(json_obj, out);
  if (json_obj["cmd"] == 1)
  {
      Serial.println("cmd = 1, need refresh");
      //fresh_sensor_info();
      int cmd = 1;
      xQueueSend(msg_queue, &cmd, 0);
  }
}


timer_func get_timer_func(int sensor_type)
{
	switch(sensor_type)
	{
		case SENSOR_TEMPER:
			return sensor_temper_func;
			break;
		case SENSOR_HEART_BEAT:
			return sensor_heart_beat;
			break;
	}
	return NULL;
}

const char* host = "192.168.10.105";
const int httpPort = 80;
const char* streamId   = "....................";
const char* privateKey = "....................";
int wifi_connect_times = 20;
int http_connect_times = 10;
int temper_report_interval = 600; //s
int water_level_report_interval = 600;
int board_sensor_report_interval = 600;
int air_pressure_report_interval = 60;

sensor_info_t *sensor_array = NULL;

int http_send(String url, String &out)
{
  Serial.println("int http send");
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  
  if (!client.connect(host, httpPort)) {
      http_connect_times--;
      Serial.println("connection failed");
      return -1;
      
  }

  // We now create a URI for the request
  
  Serial.println("Requesting URL: " + url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
      vTaskDelay(500);
      if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return -2;
      }
  }

  // Read all the lines of the reply from server and print them to Serial
  while(client.available()) {
      String line = client.readStringUntil('\r');
      out = line;
  }
  Serial.println("out:"+out);
  return 0;
}

int report_sensor_info()
{
	String url = "/portal_cgi?opt=report_board_sensor&client_mac="+dev_mac+"&sensor_json=";
	DynamicJsonDocument json_obj(1024);
	util_timer *t = NULL;
	int index = 0;
	list_for_each_entry(t, &my_timer_list, list) {
    if (t->timer_type >= 100)
      continue;
		json_obj["sensor_array"][index]["id"] = t->id;
		json_obj["sensor_array"][index]["report_interval"] = t->interval;
		json_obj["sensor_array"][index]["pool_id"] = t->pool_id;
		json_obj["sensor_array"][index]["sensor_pin"] = t->pin;
		json_obj["sensor_array"][index]["other_param"] = t->other_param;
		json_obj["sensor_array"][index]["type"] = t->timer_type;
    index++;
	}
	String json_str;
	serializeJson(json_obj, json_str);
	url = url+json_str;
	String out;
	http_send(url, out);
}

int fresh_sensor_info()
{
	String url = "/portal_cgi?opt=query_sensor_by_mac&client_mac="+dev_mac;
    String out = "";

	http_send(url, out);
	String json_str;
	DynamicJsonDocument json_obj(1024);
	deserializeJson(json_obj, out);

	util_timer *t = NULL;
	for (int i = 0; i < json_obj["data"].size(); i++)
	{
		int pin = atoi(json_obj["data"][i]["sensor_pin"]);
		t = find_timer(pin);
		if (t)
		{
			t->id = atoi(json_obj["data"][i]["id"]);
			t->timer_type = atoi(json_obj["data"][i]["type"]);
			t->interval = atoi(json_obj["data"][i]["report_interval"]);
			strncpy(t->other_param, json_obj["data"][i]["other_param"], sizeof(t->other_param));
			t->pin = pin;
		}else
		{
			t = (util_timer *)malloc(sizeof(util_timer));
      t->id = atoi(json_obj["data"][i]["id"]);
      t->pool_id = atoi(json_obj["data"][i]["pool_id"]);
			t->timer_type = atoi(json_obj["data"][i]["type"]);
			t->cb_func = get_timer_func(t->timer_type);
			t->pin = pin;
			t->expire = millis() + 20*1000;
			t->interval = atoi(json_obj["data"][i]["report_interval"]);
			t->loop = 1;
      strncpy(t->other_param, json_obj["data"][i]["other_param"], sizeof(t->other_param));
      Serial.println("5");
			list_add(&t->list, &my_timer_list);
		}
	}
  Serial.println("after modify");
	util_timer *p = NULL;
	util_timer *n = NULL;
	list_for_each_entry_safe(p, n, &my_timer_list, list) {
		if (p->timer_type == SENSOR_HEART_BEAT)
			continue;
		int exist = 0;
		for (int i = 0; i < json_obj["data"].size(); i++)
		{
			if (p->pin == atoi(json_obj["data"][i]["sensor_pin"]))
			{
				exist = 1;
				break;
			}
		}
		if (exist == 0) {
			list_del(&p->list);
			Serial.println("del timer:"+ p->pin);
			free(p);
		}			
	}
	report_sensor_info();
	return 0;
}

int wifi_connect()
{
	vTaskDelay(10);
	Serial.print("Connecting to ");
    Serial.println(ssid);
	WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        wifi_connect_times--;
        vTaskDelay(500);
        if (wifi_connect_times < 0)
        {
          wifi_connect_times = 20;
          vTaskDelay(20000); //连不上wifi睡眠20s
          return -1;
        }
        Serial.print(".");
    }
	Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
  byte mac[6];
  WiFi.macAddress(mac);
  dev_mac =  String(mac[0], HEX)+":"+String(mac[1], HEX)+":"+String(mac[2], HEX)+
              ":"+String(mac[3], HEX)+":"+String(mac[4], HEX) +":"+String(mac[5], HEX);
  Serial.println("mac:"+dev_mac);
	return 0;
}

void main_task( void * parameter )
{
  vTaskDelay(10);
	msg_queue = xQueueCreate(10, sizeof(msg_item_t));
	while(wifi_connect() != 0)
	{
	}
  fresh_sensor_info();
	util_timer *t = (util_timer *)malloc(sizeof(util_timer));
	t->timer_type = SENSOR_HEART_BEAT;
	t->cb_func = get_timer_func(t->timer_type);
	t->pin = 100;
	t->expire = millis() + 20*1000;
	t->interval = HEART_BEAT_INTERVAL;
	t->loop = 1;
	list_add(&t->list, &my_timer_list);
  int cmds[100];
  int ret = 0;
	while (1)
	{
      
    ret = xQueueReceive(msg_queue, cmds, 1000);
    if (ret > 0) {
      for(int i = 0; i < ret; i++) {
        switch(cmds[i]) {
        case 1:
          fresh_sensor_info();
          break;
        }
      }
    }
		
		timer_handler();
	}

	
}
 
void task2( void * parameter)
{
    char out_str[100] = {0};
    for( int i = 0;i<100;i++ ){
        sprintf(out_str, "Hello from task 2 %d", i);
        Serial.println(out_str);
        vTaskDelay(100000); 
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

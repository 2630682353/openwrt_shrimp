#include <dummy.h>

#include <AsyncUDP.h>

#include <WiFi.h>

#define PIN_NUM 40
const char* ssid     = "zc_test";
const char* password = "58285390";
struct list_head my_timer_list;
char *sensor_table[PIN_NUM];

typedef struct sensor_info_st
{
	int report_interval;
	unsigned char type;
	unsigned char pin;
	unsigned char pool_id;
}sensor_info_t;

typedef struct msg_item_st
{
	int msg_type;
}msg_item_t;


typedef struct util_timer_st {
	time_t expire;
	int (*cb_func)(void *para);
	int loop;
	int id;
	int pin;
	int interval;
	int timer_type;
	String other_param;
	struct list_head list;
}util_timer;

static LIST_HEAD(my_timer_list); 
static int timer_timeslot;
static pthread_mutex_t timer_mutex;

void timer_handler() {
	time_t cur = uptime();
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
				p->expire = cur + p->interval;
			}
		}
	}
}

util_timer *add_timer(int (*cb_func)(),int delay,int loop, int interval, void *para, int type) {
	util_timer *t = malloc(sizeof(util_timer));
	t->cb_func = cb_func;
	t->expire = uptime() + delay;
	t->interval = interval;
	t->loop = loop;
	t->para = para;
	t->timer_type = type;
	list_add(&t->list, &my_timer_list);
	return t;
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

int timer_list_init()
{
	pthread_mutex_init(&timer_mutex, NULL);
	return 0;
}

util_timer* find_board(int pin)
{
	util_timer *t = NULL;
	list_for_each_entry(t, &my_timer_list, list) {
		if (t->, client_mac) == 0) {
			return p;
		}
	}
	return p;
}


const char* host = "192.168.10.103";
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
char *dev_mac = "aa:aa:aa:aa:aa:aa"

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
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
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

  Serial.println();
  Serial.println("closing connection");
  return 0;
}

int fresh_sensor_info()
{
	String url = "/portal_cgi?opt=query_temper&client_mac="+dev_mac;
    String out = "";

	http_send(url, out);
	String json_str;
	DynamicJsonDocument json_obj(1024);
	deserializeJson(json_obj, out);
	serializeJson(json_obj, json_str);

	for (int i = 0; i < json_obj["data"].length; i++)
	{
		sensor_info_t *sensor_info = malloc(sizeof(sensor_info_t));
		sensor_info->pin = json_obj["data"][i]["sensor_pin"].toInt();
		sensor_info->type = json_obj["data"][i]["type"].toInt();
		sensor_table[sensor_info->pin] = sensor_info;
		
	}
	util_timer *t = NULL;
	list_for_each_entry(t, &my_timer_list, list) {
		if (strcmp(p->mac, client_mac) == 0) {
			if (!list_empty(&p->task_list)) {
				list_for_each_entry_safe(task, task2, &p->task_list, task_list) {
					if (strcmp(task->task_name, task_name) == 0 && task->task_id == task_id) {
						list_del(&task->task_list);
						free(task);
						break;
					}				
				}
			}
			break;
		}
	}

	Serial.println("out:"+ out);
	vTaskDelay(5000);
}

int get_board_sensor_info()
{
	String sensor_json = "";
	if (http_send("/portal_cgi?opt=query_sensor&client_mac=all", sensor_json) == 0)
	{
		DynamicJsonDocument doc(1024);
		deserializeJson(doc, sensor_json);
		int sensor_pin = doc["data"][0]["sensor_pin"];
		
	}


}

void main_task( void * parameter )
{
    vTaskDelay(10);
	char *msg_buf[16] = {0};
	msg_queue = xQueueCreate(10, sizeof(msg_item_t));
    wifi_connect_times = 20;
    http_connect_times = 10;
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        wifi_connect_times--;
        vTaskDelay(500);
        if (wifi_connect_times < 0)
          return;
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    
	while (1)
	{
		xQueueReceive(msg_queue, msg_buf, 1000);
		timer_handler();
	}

	
}
 
void task3()
{
	keyVal = KEY_Scan(0);
    if(keyVal == KEY0_PRES)         //启动定时器
    {
       xTimerStart(heart_beat_report_timer,0);
       xTimerStart(heart_beat_report_timer,0);
    }
    if(keyVal == KEY1_PRES)         //关闭定时器
    {
       xTimerStop(heart_beat_report_timer,0);
       xTimerStop(heart_beat_report_timer,0); 
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
  for (int i=0; i< PIN_NUM;i++)
  {
	sensor_table[i] = NULL;
  }
  
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

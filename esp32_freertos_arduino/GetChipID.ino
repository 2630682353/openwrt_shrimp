#include <dummy.h>

#include <AsyncUDP.h>

#include <WiFi.h>

const char* ssid     = "zc_test";
const char* password = "58285390";
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
	int interval;
	int timer_type;
	void *para;
	struct list_head list;
}util_timer;

static LIST_HEAD(my_timer_list); 
static int timer_timeslot;
static pthread_mutex_t timer_mutex;

void timer_handler() {
	time_t cur = uptime();
	util_timer *p = NULL;
	util_timer *n = NULL;
	pthread_mutex_lock(&timer_mutex);
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
	pthread_mutex_unlock(&timer_mutex);
}

util_timer *add_timer(int (*cb_func)(),int delay,int loop, int interval, void *para, int type) {
	util_timer *t = malloc(sizeof(util_timer));
	t->cb_func = cb_func;
	t->expire = uptime() + delay;
	t->interval = interval;
	t->loop = loop;
	t->para = para;
	t->timer_type = type;
	pthread_mutex_lock(&timer_mutex);
	list_add(&t->list, &my_timer_list);
	pthread_mutex_unlock(&timer_mutex);
	return t;
}

int del_timer(int type)
{
	util_timer *p = NULL;
	util_timer *n = NULL;
	pthread_mutex_lock(&timer_mutex);
	list_for_each_entry_safe(p, n, &my_timer_list, list) {
		if (p->timer_type == type) {
			list_del(&p->list);
			if (p->para)
				free(p->para);
			free(p);
		}			
	}
	pthread_mutex_unlock(&timer_mutex);
	return 0;
}

int timer_list_init()
{
	pthread_mutex_init(&timer_mutex, NULL);
	return 0;
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
TimerHandle_t temper_report_timer;
TimerHandle_t water_level_report_timer;
TimerHandle_t board_sensor_report_timer;
TimerHandle_t heart_beat_report_timer;

sensor_info_t *sensor_array = NULL;


BaseType_t xTimerChangePeriod( TimerHandle_t xTimer,
                            TickType_t xNewTimerPeriodInTicks,
                            TickType_t xTicksToWait );

void pxSoftWaveTimer(TimerHandle_t xTimer); //软件定时器回调函数

SoftWaveTimer1 = xTimerCreate(
							 "heart_beat_timer",
							 1000,
							 pdTRUE,
							 (void*)1,
							 heart_beat_report_timer);
 SoftWaveTimer2 = xTimerCreate(
							 "board_sensor_report_timer",	//定时器句柄
							 3000,				//定时器周期
							 pdTRUE,			//周期/单次定时器
							 (void*)2,			//定时器ID
							 board_sensor_report_timer);	//回调函数指针


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
    String url = "/portal_cgi?opt=query_temper&client_mac=all&period=recent&client_temper_index=all";
    String out = "";
    while(1)
    {
      http_send(url, out);
	  String json_str;
	  DynamicJsonDocument json_obj(1024);
		deserializeJson(json_obj, out);
		serializeJson(json_obj, json_str);

		const char* sensor = json_obj["sensor"];
		long time          = json_obj["time"];
		double latitude    = json_obj["data"][0];
		double longitude   = json_obj["data"][1];
      Serial.println("out:"+ out);
      vTaskDelay(5000);
    } 
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

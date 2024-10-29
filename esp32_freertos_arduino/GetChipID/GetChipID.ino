#include <dummy.h>

#include <AsyncUDP.h>
#include "list.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <string.h>
#include "time.h"
#include <LinkedList.h>
#include "HX711.h"
#include <OneWire.h>
#include "DHTesp.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "HX710B.h"

const int FEED_WEIGHT_DOUT_PIN = 5;
const int FEED_WEIGHT_SCK_PIN = 4;
int TEMPER_PIN = 8;

#define TWDT_TIMEOUT_MS         15000

#define PIN_NUM 40
#define HEART_BEAT_INTERVAL 20
#define REPORT_SENSOR_INFO_INTERVAL 3600
const char* ssid     = "OpenWrt-test24";
//const char* ssid     = "WIFI_F7CE";
const char* password = "58285390";
String dev_mac = "aa:aa:aa:aa:aa:aa";
TaskHandle_t handle_1 = NULL;
TaskHandle_t handle_2 = NULL;

const char* ntpServer = "ntp.aliyun.com";
const long  gmtOffset_sec = 3600*8;
const int   daylightOffset_sec = 0;
ComfortState cf;
DHTesp dht;

HX711 scale;
HX710B pressure_sensor; 
double pressure;
double pressure_offset = 37.17;
int outer_cmd = 0;

QueueHandle_t msg_queue;
LIST_HEAD(my_timer_list);

enum SENSOR_TYPE
{
	SENSOR_TEMPER = 1,
  SENSOR_AIR_PRESSURE,
	SENSOR_WEIGHT,
	SENSOR_WATER_LEVEL,
	SENSOR_ELEC,
  SENSOR_TEMPER_HUMIDITY,

	
	SENSOR_FEED_SPECIFY_TIME = 100,

  SENSOR_HEAT_TEMPER = 150,
  SENSOR_SWITCH = 180,
	
	SENSOR_HEART_BEAT = 200,
	SENSOR_REPORT_SENSOR_INFO = 201,
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
	unsigned char loop;
	unsigned char pin;
	unsigned char timer_type;
	unsigned char pool_id;
	int id;
	int interval;
	char other_param[64];
  	char *para;
	struct list_head list;
	float feed_weight;
  unsigned char on_state;
  unsigned char weight_pin;
}util_timer;

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return;
}

unsigned long  calculate_expire(char *time_str_in)
{
	unsigned long cur = millis();
	char time_str[64] = {0};
	if (strlen(time_str_in) == 0)
		return cur+365*24*3600*1000*10;
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo)){
		Serial.println("Failed to obtain time");
		return cur+1000*60;;
	}
	strncpy(time_str, time_str_in, sizeof(time_str));
	LinkedList<int> time_list = LinkedList<int>();
	char *outer_ptr = NULL;
	char *outer_ptr2 = NULL;
	char *split_str = NULL;
	char *split_str2 = NULL;
	int hour = 0, minute = 0;
	split_str = strtok_r(time_str, ",", &outer_ptr);
	while(split_str != NULL)
	{
		split_str2 = strtok_r(split_str, ":", &outer_ptr2);
		if (split_str2)
			hour = atoi(split_str2);
		split_str2 = strtok_r(NULL, ":", &outer_ptr2);
		if (split_str2)
			minute = atoi(split_str2);
		time_list.add(hour*3600+minute*60);
    split_str = strtok_r(NULL, ",", &outer_ptr);
	}
	int now_sec = timeinfo.tm_hour*3600+timeinfo.tm_min*60+timeinfo.tm_sec;
  
	for(int i = 0; i < time_list.size(); i++){
		if (now_sec+1 < time_list.get(i)){
			return cur+(time_list.get(i)-now_sec)*1000;
    }
		else if(i == time_list.size() - 1)
			return (24*3600-now_sec+time_list.get(0))*1000;
	}
	
}


void timer_handler() {
	unsigned long cur = millis();
	util_timer *p = NULL;
	util_timer *n = NULL;
	list_for_each_entry_safe(p, n, &my_timer_list, list) {
		if (cur >= p->expire) {
			p->cb_func((void *)p);
			if (!p->loop) { 
				list_del(&p->list);
				if (p->para)
					free(p->para);
				free(p);
			}
			else {
				if (p->timer_type >= 100 && p->timer_type < 150)
				{
					p->expire = calculate_expire(p->other_param);
				}else
				{
					p->expire = cur + p->interval*1000;
				}
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

util_timer* find_timer_by_type(int type)
{
	util_timer *t = NULL;
	list_for_each_entry(t, &my_timer_list, list) {
		if (t->timer_type == type) {
			return t;
		}
	}
	return NULL;
}

float getTemp(OneWire &ds){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
  
}

int sensor_temper_func(void *param)
{
  util_timer *p_timer = (util_timer *)param;
  OneWire ds(p_timer->pin);
  float temper = getTemp(ds);
  if (temper < -100)
    temper = 0.0;
  String url = "/portal_cgi?opt=update_temper&client_mac="+dev_mac+"&sensor_pin="+p_timer->pin+
				"&temper="+temper;
	String out;
	http_send(url, out);
  util_timer *p_heat_sensor = find_timer_by_type(SENSOR_HEAT_TEMPER);
  if (p_heat_sensor != NULL && strlen(p_heat_sensor->other_param) > 0) {
    char temp[64] = {0};
    int low_temper, high_temper;
    strcpy(temp, p_heat_sensor->other_param);
    char *low = strtok(temp, ",");
    if (low)
      low_temper = atoi(low);
    char *high = strtok(NULL, ",");
    if (high)
      high_temper = atoi(high);
    char *switch_mac = strtok(NULL, ",");
    char *switch_pin = strtok(NULL, ",");
    String str = "low_temper:";
    str = str+low_temper+"+high_temper:"+high_temper;
    Serial.println(str);
    url = "/portal_cgi?opt=get_switch_status&client_mac="+dev_mac+"&switch_mac="+switch_mac+
				"&switch_pin="+switch_pin;
    
    DynamicJsonDocument json_obj(4096);
    if (http_send(url, out) < 0)
        return -1;
    if (deserializeJson(json_obj, out))
        return -1;
    if (json_obj["code"] != 0)
        return -1;
    if (strcmp("switch_off", json_obj["data"]) == 0)
      p_heat_sensor->on_state = 0; //开关关闭
    else if(strcmp("switch_on", json_obj["data"]) == 0)
      p_heat_sensor->on_state = 1; //开关开启

    if (temper < low_temper && p_heat_sensor->on_state == 0) {
      url = "/portal_cgi?opt=board_message&client_mac="+dev_mac+"&dst_mac="+switch_mac+
				"&message_type=heat_on";
        http_send(url, out);
    }
    else if (temper > high_temper && p_heat_sensor->on_state == 1) {
      url = "/portal_cgi?opt=board_message&client_mac="+dev_mac+"&dst_mac="+switch_mac+
				"&message_type=heat_off";
        http_send(url, out);
    }
  }
  return 0;
  
}

int sensor_air_pressure_func(void *param)
{
  util_timer *p_timer = (util_timer *)param;
  pressure_sensor.begin(p_timer->pin, p_timer->pin+1, 128);
  vTaskDelay(200);
  if (pressure_sensor.is_ready()) {
    
    pressure = pressure_sensor.pascal() + pressure_offset;
  } else {
    pressure = 0.00;
  }
  String url = "/portal_cgi?opt=update_air_pressure&client_mac="+dev_mac+"&sensor_pin="+p_timer->pin+
				"&pressure="+pressure;
	String out;
	http_send(url, out);
  return 0;
}

int report_work_log(char *log_type, char *message)
{
  String url = "/portal_cgi?opt=work_log&client_mac="+dev_mac+"&log_type="+log_type+
				"&message="+message;
	String out;
	http_send(url, out);
  return 0;
}

int sensor_heat_temper_func(int enable)
{
  util_timer *p_timer = find_timer(15); //加热开关15引脚
  if (p_timer == NULL)
    return -1;
  if (enable == 1) {
    pinMode(p_timer->pin, OUTPUT);
    digitalWrite(p_timer->pin, HIGH);
    p_timer->on_state = 1;
    strcpy(p_timer->other_param, "switch_on");
    report_sensor_info(NULL);
  }else{
    pinMode(p_timer->pin, OUTPUT);
    digitalWrite(p_timer->pin, LOW);
    p_timer->on_state = 0;
    strcpy(p_timer->other_param, "switch_off");
    report_sensor_info(NULL);
  }
  return 0;
}

int sensor_air_temper_humidity_func(void *param)
{
  util_timer *p_timer = (util_timer *)param;
  dht.setup(p_timer->pin, DHTesp::DHT11);

  // Reading temperature for humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
	// Check if any reads failed and exit early (to try again).
	if (dht.getStatus() != 0) {
		Serial.println("DHT11 error status: " + String(dht.getStatusString()));
		return -1;
	}

	float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };
  
  String url = "/portal_cgi?opt=update_air_temper_humidity&client_mac="+dev_mac+"&sensor_pin="+p_timer->pin+
        "&temper="+newValues.temperature+"&humidity="+newValues.humidity;
  String out;
  http_send(url, out);
  return 0;
}

int sensor_water_level_func(void *param)
{
  util_timer *p_timer = (util_timer *)param;
  float water_level = millis()%10 + 10.0;
  String url = "/portal_cgi?opt=update_water_level&client_mac="+dev_mac+"&sensor_pin="+p_timer->pin+
				"&water_level="+water_level;
	String out;
	http_send(url, out);
  return 0;
}

int sensor_elec_func(void *param)
{
  util_timer *p_timer = (util_timer *)param;
  float elec = millis()%10 + 100.0;
  String url = "/portal_cgi?opt=update_elec&client_mac="+dev_mac+"&sensor_pin="+p_timer->pin+
				"&elec="+elec;
	String out;
	http_send(url, out);
  return 0;
}

int sensor_weight_func(void *param)
{
  float feed_weight;
  util_timer *p_timer = (util_timer *)param;
  scale.begin(p_timer->pin, p_timer->pin + 1); //(dout, sck)
  
  vTaskDelay(200);
  if (scale.is_ready()) {
    feed_weight = scale.get_units(10);
  } else {
    Serial.println("HX711 not found.");
	return -1;
  }
	String url = "/portal_cgi?opt=update_feed_weight&client_mac="+dev_mac+"&sensor_pin="+p_timer->pin+
				"&feed_weight="+feed_weight;
	String out;
	http_send(url, out);
  return 0;
}
int sensor_feed_func(void *param)
{
  util_timer *p = (util_timer *)param;
  float before_weight;
  float after_weight;
  Serial.println("in motor func");
  printLocalTime();
  
  if (scale.is_ready()) {
    before_weight = scale.get_units(10);
  } else {
    Serial.println("HX711 not found.");
	return -1;
  }
  after_weight = before_weight;
  //motor_start
  pinMode(p->pin, OUTPUT);
  digitalWrite(p->pin, HIGH);
  while(before_weight - after_weight < p->feed_weight)
  {
    if (scale.is_ready()) {
        after_weight = scale.get_units(10);
      Serial.print("after_weight:");
      Serial.println(after_weight);
      vTaskDelay(200);
    } else {
      Serial.println("HX711 not found.");
      break;
    }
  }
  //motor_end;
  pinMode(p->pin, OUTPUT);
  digitalWrite(p->pin, LOW);
  return 0;
  
}



int sensor_heart_beat(void *param)
{
	String url = "/portal_cgi?opt=heart_beat&client_mac="+dev_mac;
  int ret = 0;
	DynamicJsonDocument json_obj(1024);
	String out;
	if (http_send(url, out) < 0)
      return -1;
  if (deserializeJson(json_obj, out))
      return -1;
  if (json_obj["cmd"] == 1)
  {
      Serial.println("cmd = 1, need refresh");
      //fresh_sensor_info();
      outer_cmd = 1;
  }else if (json_obj["cmd"] == 101 || json_obj["cmd"] == 102)
  {
      Serial.println("cmd = ");
      //Serial.println(json_obj["cmd"]);
      //fresh_sensor_info();
      outer_cmd = json_obj["cmd"];
  }
  return 0;
}


timer_func get_timer_func(int sensor_type)
{
	switch(sensor_type)
	{
		case SENSOR_TEMPER:
			return sensor_temper_func;
			break;
    case SENSOR_AIR_PRESSURE:
			return sensor_air_pressure_func;
			break;
    case SENSOR_WATER_LEVEL:
			return sensor_water_level_func;
			break;
    case SENSOR_ELEC:
      return sensor_elec_func;
      break;
		case SENSOR_HEART_BEAT:
			return sensor_heart_beat;
			break;
    case SENSOR_FEED_SPECIFY_TIME:
			return sensor_feed_func;
			break;
    case SENSOR_TEMPER_HUMIDITY:
      return sensor_air_temper_humidity_func;
      break;
    case SENSOR_WEIGHT:
      return sensor_weight_func;
      break;
	}
	return NULL;
}

const char* host = "124.221.58.186";
const int httpPort = 32844;
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
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  //const int httpPort = 80;
  http_connect_times = 5;
  while (!client.connect(host, httpPort)) {
      http_connect_times--;
      vTaskDelay(1000);
      Serial.println("connection failed");
      if (http_connect_times < 0) {
		if (WiFi.status() != WL_CONNECTED)
			ESP.restart();
	return -1;		
	}
        
      
  }

  // We now create a URI for the request
  
  Serial.println("Requesting URL: " + url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
       
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

int report_sensor_info(void *param)
{
	String url = "/portal_cgi?opt=report_board_sensor&client_mac="+dev_mac+"&sensor_json=";
	DynamicJsonDocument json_obj(4096);
	util_timer *t = NULL;
	int index = 0;
	list_for_each_entry(t, &my_timer_list, list) {
    if (t->timer_type >= SENSOR_HEART_BEAT)
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
  return 0;
}

int split_feed_other_param(util_timer *t, const char *other_param)
{
  char temp[64] = {0};
  strcpy(temp, other_param);
  char *weight_pin_s = strtok(temp, "|");
  if (weight_pin_s) 
    t->weight_pin = atoi(weight_pin_s);
  char *feed_weight_s = strtok(NULL, "|");
  if (feed_weight_s)
    t->feed_weight = atoi(feed_weight_s);
  char *specify_time = strtok(NULL, ",");
  if (specify_time)
    strcpy(t->other_param, specify_time);
}

int fresh_sensor_info()
{
	String url = "/portal_cgi?opt=query_sensor_by_mac&client_mac="+dev_mac;
  String out = "";

	String json_str;
	DynamicJsonDocument json_obj(4096);
	if (http_send(url, out) < 0)
      return -1;
  if (deserializeJson(json_obj, out))
      return -1;

	util_timer *t = NULL;
	for (int i = 0; i < json_obj["data"].size(); i++)
	{
		int pin = atoi(json_obj["data"][i]["sensor_pin"]);
		t = find_timer(pin);
		if (t)
		{
			t->id = atoi(json_obj["data"][i]["id"]);
			t->timer_type = atoi(json_obj["data"][i]["type"]);
      t->pool_id = atoi(json_obj["data"][i]["pool_id"]);
			t->interval = atoi(json_obj["data"][i]["report_interval"]);
      if (t->timer_type == SENSOR_FEED_SPECIFY_TIME) {
        split_feed_other_param(t, json_obj["data"][i]["other_param"]);
      }
      else {
			  strncpy(t->other_param, json_obj["data"][i]["other_param"], sizeof(t->other_param));
      }
			t->pin = pin;
      if (t->timer_type >= 100 && t->timer_type < 150)  //定时任务
        t->expire = calculate_expire(t->other_param);
      else if (t->timer_type >= 150 && t->timer_type < 200) {  //不需要周期执行任务
        t->expire = millis() + 365*24*60*60*1000;
      }else{
        t->expire = millis() + t->interval*1000;
      }
		}else
		{
			t = (util_timer *)malloc(sizeof(util_timer));
      memset(t, 0, sizeof(util_timer));
      t->id = atoi(json_obj["data"][i]["id"]);
      t->pool_id = atoi(json_obj["data"][i]["pool_id"]);
			t->timer_type = atoi(json_obj["data"][i]["type"]);
      if (t->timer_type == SENSOR_HEAT_TEMPER)
        t->cb_func = NULL;
      else
			  t->cb_func = get_timer_func(t->timer_type);
			t->pin = pin;
      strncpy(t->other_param, json_obj["data"][i]["other_param"], sizeof(t->other_param));
      if (t->timer_type >= 100 && t->timer_type < 150)  //定时任务
        t->expire = calculate_expire(t->other_param);
      else if (t->timer_type >= 150 && t->timer_type < 200) {  //不需要周期执行任务
        t->expire = millis() + 365*24*60*60*1000;
        t->on_state = 0;
        if (t->timer_type == SENSOR_SWITCH) {
          strncpy(t->other_param, "switch_off", sizeof(t->other_param));
          pinMode(t->pin, OUTPUT);
          digitalWrite(t->pin, LOW);
        }
      }else
			  t->expire = millis() + 20*1000;
			t->interval = atoi(json_obj["data"][i]["report_interval"]);
			t->loop = 1;
      
      if (t->timer_type == SENSOR_WEIGHT)
      {
        scale.begin(t->pin, t->pin + 1); //(dout, sck)
        scale.set_scale(199.6718);    //只需要改这个值 this value is obtained by calibrating the scale with known weights; see the README for details
        //scale.tare();	
      }
      Serial.println("5");
			list_add(&t->list, &my_timer_list);
		}
	}
  Serial.println("after modify");
	util_timer *p = NULL;
	util_timer *n = NULL;
	list_for_each_entry_safe(p, n, &my_timer_list, list) {
		if (p->timer_type >= SENSOR_HEART_BEAT)
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
	report_sensor_info(NULL);
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
  int ret = 0;
  
	while(wifi_connect() != 0)
	{
	}
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  	printLocalTime();
  ret = fresh_sensor_info();
  while(ret != 0) {
    vTaskDelay(10000);
    ret = fresh_sensor_info();
  }
	util_timer t;
	t.timer_type = SENSOR_HEART_BEAT;
	t.cb_func = get_timer_func(t.timer_type);
	t.pin = 100;
	t.expire = millis() + 20*1000;
	t.interval = HEART_BEAT_INTERVAL;
	t.loop = 1;
	list_add(&t.list, &my_timer_list);

	util_timer t2;
	t2.timer_type = SENSOR_REPORT_SENSOR_INFO;
	t2.cb_func = report_sensor_info;
	t2.pin = 201;
	t2.expire = millis() + REPORT_SENSOR_INFO_INTERVAL*1000;
	t2.interval = REPORT_SENSOR_INFO_INTERVAL;
	t2.loop = 1;
	list_add(&t2.list, &my_timer_list);

  int cmds[100];

  
  
  esp_task_wdt_reset();
	while (true)
	{
    if (outer_cmd == 1) {
      fresh_sensor_info();
      outer_cmd = 0;
    }else if(outer_cmd == 101) {
      sensor_heat_temper_func(1); //开启加热
      outer_cmd = 0;
    }else if(outer_cmd == 102) {
      sensor_heat_temper_func(0); //关闭加热
      outer_cmd = 0;
    }
    
		vTaskDelay(1000);
		timer_handler();
    esp_task_wdt_reset();

	}

	
}
 
void task2( void * parameter)
{
    char str[100] = {0};
    for( int i = 0;i<1000;i++ ){
        sprintf(str, "Hello from task 2 %d",i);
        Serial.println(str);
        vTaskDelay(100000); 
    }
}
 
void setup() {
  Serial.begin(115200);
  
 // xTaskCreatePinnedToCore(main_task, "TaskMain", 2048, xTaskGetCurrentTaskHandle(), 10, &handle_1, 0);
  xTaskCreate(
                    main_task,          //指定任务函数，也就是上面那个task1函数
                    "TaskMain",        //任务名称
                    20480,            //任务堆栈大小
                    NULL,             //作为任务输入传递的参数
                    1,                //优先级
                    &handle_1);            //任务句柄
 
  xTaskCreate(
                    task2,         
                    "TaskTwo",        
                    2048,            
                    NULL,             
                    1,               
                    NULL); 
  esp_task_wdt_deinit();
  esp_task_wdt_init(TWDT_TIMEOUT_MS, true);
  esp_task_wdt_add(handle_1);
  
                  
}
 
 
void loop() {

}

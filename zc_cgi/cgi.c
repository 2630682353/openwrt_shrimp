#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cJSON.h"
#include "message.h"
#include "sock.h"
#include "log.h"
#include "tools.h"
#include "protocol.h"
#include "connection.h"
#include "libcom.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sqlite3.h>

extern sqlite3 *pdb;
extern v_list_t head;
extern struct list_head board_list;
char *errmsg = NULL;

enum {

CGI_ERR_NAMEPASSWD = 10001,
CGI_ERR_OTHER = 10002
};

typedef struct user_info
{
	char name[20];		//字符串
	char pwd[20];		//字符串
	unsigned char mac[6];
	uint32 ipaddr;
}user_info_t;

typedef struct user_tel_info
{
	char tel[20];		//字符串
	char pwd[20];
	unsigned char mac[6];
}user_tel_info_t;

enum {
	CGI_ERR_FAIL = 10001,
	CGI_ERR_INPUT,
	CGI_ERR_MALLOC,
	CGI_ERR_EXIST,
	CGI_ERR_NONEXIST,
	CGI_ERR_FULL,
	CGI_ERR_NOLOGIN,
	CGI_ERR_NOSUPPORT,
	CGI_ERR_ACCOUNT_NOTREADY,
	CGI_ERR_TIMEOUT,
	CGI_ERR_FILE,
	CGI_ERR_RULE,
};

int query_table(char *sql, int r, int c)
{
    char *errMsg;
    char **dbResult;
    int nRow = 0, nColumn = 0;
    int rc;

    int result;
    rc = sqlite3_get_table(pdb,sql,&dbResult,&nRow,&nColumn,&errMsg);
    if(rc == SQLITE_OK && r<=nRow &&r>0 && c<=nColumn)
    {
        result = atoi(dbResult[r*nColumn+c]);
        sqlite3_free_table(dbResult);
        return result;
    }
}

void strlower(char *s)
{
	int i;
	for(i=0;i<strlen(s);i++)//此处要从0开始计数，因为字符串第一个字符是s[0]
	{
		if(*(s+i)>=65 && *(s+i)<=92)
			*(s+i)+=32;
	}
}

int cgi_free_rcvbuf(void *rcv_buf)
{
	msg_t *msg = container_of(rcv_buf, msg_t, data);
	free((void *)msg);
	return 0;
}

int query_data_to_json(void *para,int ncol,char *col_val[],char ** col_name)
{
	cJSON *array, *item;
	int i;
	array = (cJSON *)para;
	item = cJSON_CreateObject();
	for(i=0;i<ncol;i++)                 //打印值
	{
		cJSON_AddStringToObject(item, col_name[i], col_val[i]);
	}
	cJSON_AddItemToArray(array, item);
	return 0;
}

#if 0
int http_send(char *url, cJSON *send, cJSON **recv, char *http_headers[])
{
	int ret = -1;
	char *jstr = NULL, *back_str = NULL;
	cJSON *obj = NULL;
	back_str = (char*)malloc(4096);
	memset(back_str, 0, 4096);
	
	char header_str[256] = {0};

	struct curl_slist *headers = NULL;
	CURLcode res = CURLE_OK;
	CURL *mycurl = curl_easy_init();
	snprintf(header_str, sizeof(header_str) - 1, "DevMac: %s", gateway.mac);
	headers = curl_slist_append(headers, header_str);
	if (http_headers) {
		int i = 0;
		while (http_headers[i]) {
			headers = curl_slist_append(headers, http_headers[i]);
			i++;
		}

	}
		
	if (!mycurl)
		goto out;
	curl_easy_setopt(mycurl, CURLOPT_URL, url);
	curl_easy_setopt(mycurl, CURLOPT_TIMEOUT, HTTP_TIMEOUT); 
	curl_easy_setopt(mycurl, CURLOPT_WRITEFUNCTION, receive_data);
	curl_easy_setopt(mycurl, CURLOPT_WRITEDATA, back_str);
	
	if (!send) {
		curl_easy_setopt(mycurl, CURLOPT_HTTPHEADER, headers);
		res = curl_easy_perform(mycurl);

	} else {
		snprintf(header_str, sizeof(header_str) - 1, "Content-Type:application/json");
		headers = curl_slist_append(headers, header_str); 
		jstr = cJSON_PrintUnformatted(send); 
		GATEWAY_LOG(LOG_DEBUG, "http send %s\n", jstr);
		curl_easy_setopt(mycurl, CURLOPT_HTTPHEADER, headers); 
		curl_easy_setopt(mycurl, CURLOPT_POSTFIELDS, jstr); 
		res = curl_easy_perform(mycurl);
	}
	if (res != CURLE_OK) {
		GATEWAY_LOG(LOG_WARNING, "curl_easy_perform() failed: %d\n", res);
		goto out;
    }
	GATEWAY_LOG(LOG_DEBUG, "http recv %s\n", back_str);
	obj = cJSON_Parse(back_str);
	if (!obj)
		goto out;
	cJSON *result = cJSON_GetObjectItem(obj, "result");
	if (!result)
		goto out;
	ret = result->valueint;
	if (ret)
		goto out;
	*recv = obj;
out:
	if (back_str)
		free(back_str);
	if (jstr)
		free(jstr);
	if (ret) {
		recv = NULL;
		if (obj)
			cJSON_Delete(obj);
	}
	if (headers)
		curl_slist_free_all(headers);
	curl_easy_cleanup(mycurl);
	return ret;
}

int cgi_snd_msg(int cmd, void *snd, int snd_len, void **rcv, int *rcv_len)
{
	int temp_fd = 0, len = 0, ret = -1;
	char file_temp[20] = {0};
	msg_t *snd_msg = NULL;
	msg_t *rcv_msg = NULL;
	socket_t *temp_sock = NULL;
	int8 *rcv_buf = NULL;
	strcpy(file_temp, "/tmp/test.XXXXXX");
	snd_msg = malloc(sizeof(msg_t));
	
	snd_msg->cmd = cmd;
	snd_msg->dmid = MODULE_GET(cmd);
	snd_msg->dlen = snd_len;
	if ((temp_fd = mkstemp(file_temp)) < 0) {
		CGI_LOG(LOG_ERR, "mktemp sock error\n");
		goto out;
	}
	temp_sock = unix_sock_init(file_temp);
	
	sock_addr_u dst_addr;
	dst_addr.un_addr.sun_family = AF_UNIX;
	memset(dst_addr.un_addr.sun_path, 0, sizeof(dst_addr.un_addr.sun_path));
	snprintf(dst_addr.un_addr.sun_path, sizeof(dst_addr.un_addr.sun_path)-1, "/tmp/%d_rcv", MODULE_GET(snd_msg->cmd));
	if (!temp_sock)
		goto out;
	len = sock_sendmsg_unix(temp_sock, snd_msg, sizeof(msg_t), snd, snd_len, &dst_addr);
	
	if (len <= 0)
		goto out;
	rcv_buf = malloc(2048);
	len = sock_recvfrom(temp_sock, rcv_buf, 2048, NULL);
	if (len <= 0)
		goto out;
	rcv_msg = rcv_buf;
	ret = rcv_msg->result;
	if (ret || !rcv || !rcv_len)
		goto out;
	*rcv = rcv_msg->data;
	*rcv_len = rcv_msg->dlen;

out:
	if (temp_fd > 0)
		close(temp_fd);
	if (snd_msg)
		free(snd_msg);
	if (temp_sock) 
		sock_delete(temp_sock);
	if (ret) {
		if (rcv && rcv_len) {
			*rcv = NULL;
			*rcv_len = 0;
		}
		if (rcv_buf)
			free(rcv_buf);
	} else {
		if (!rcv || !rcv_len) {
			if (rcv_buf)
				free(rcv_buf);
		}
	}
	return ret;
}

int cgi_sys_auth_handler(connection_t *con)
{
	char *name = con_value_get(con,"name");
	char *pwd = con_value_get(con, "pwd");
	char *mac = con_value_get(con,"mac");
	char *vlan = con_value_get(con,"vlan");
	char *user_ip = con_value_get(con, "user_ip");
	user_query_info_t user;
	memset(&user, 0, sizeof(user));
	if (!name || !pwd || !mac || !vlan || !user_ip) {

		cJSON_AddNumberToObject(con->response, "code", 1);
		goto out;
	}
		
	strncpy(user.username, name, sizeof(user.username) - 1);
	strncpy(user.password, pwd, sizeof(user.password) - 1);
	strncpy(user.user_ip, user_ip, sizeof(user.user_ip) -1);
	strncpy(user.mac, mac, sizeof(user.mac) -1);
	user.vlan = atoi(vlan);
	
	if (cgi_snd_msg(MSG_CMD_RADIUS_USER_AUTH, &user, sizeof(user), NULL, NULL) == 0) {
		cJSON_AddNumberToObject(con->response, "code", 0);
	} else {
		cJSON_AddNumberToObject(con->response, "code", 1);
	}
	
out:
	return 1;
	
}

int cgi_sys_login_handler(connection_t *con)
{
	char *mac = con_value_get(con,"mac");
	char *ip = con_value_get(con,"ip");
	user_info_t user= {{0}, {0}, {0}, 0};
	struct in_addr user_ip;
	user_ip.s_addr = inet_addr(ip);
	
	strncpy(user.name, "18202822785", sizeof(user.name) - 1);
	strncpy(user.pwd, "1231245", sizeof(user.pwd) - 1);
	str2mac(mac, user.mac);
	user.ipaddr = user_ip.s_addr;
	
	if (cgi_snd_msg(MSG_CMD_RADIUS_USER_AUTH, &user, sizeof(user), NULL, NULL) == 0) {
		con->html_path = "portal/auth_success.html";
	} else {
		con->html_path = "portal/auth_fail.html";
	}
	
out:
	return 0;	
}

int cgi_sys_query_handler(connection_t *con)
{	
	char *mac = con_value_get(con,"mac");
	char *vlan = con_value_get(con, "vlan");
	char *user_ip = con_value_get(con, "ip");
	char *user_agent = getenv("HTTP_USER_AGENT");
	char *temp_agent = NULL;
	int os_type = -1, i = 0, ret = -1;
	char *phone_os[8] = {"iphone", "ipad", "ipod", "android", "linux", "blackberry",
						"symbianos", "windows phone"};
	char *pc_os[4] = {"mac", "hpwos", "windows", "ie"};

	user_query_info_t user;
	memset(&user, 0, sizeof(user_query_info_t));
	if (user_agent) {
		strncpy(user.user_agent, user_agent, sizeof(user.user_agent) - 1);
		strlower(user_agent);
		for (i = 0; i < 8; i++) {
			temp_agent = strstr(user_agent, phone_os[i]);
			if (temp_agent) {
				os_type = 0;
				break;
			}
		}
		if (os_type == -1) {
			for (i = 0; i < 4; i++) {
				temp_agent = strstr(user_agent, pc_os[i]);
				if (temp_agent) {
					os_type = 1;
					break;
				}
			}
		}
	}else {
		strcpy(user.user_agent, 
			"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
			"AppleWebKit/537.36 (KHTML, like Gecko) "
			"Chrome/60.0.3112.101 Safari/537.36");
	}
	
	if (!mac || !user_ip || !vlan) {
		con->html_path = "portal/error.html";
		html_tag_add(&con->tag_list, "zc:error", "error_input");
		goto out;
	}

	strncpy(user.mac, mac, sizeof(user.mac) - 1);
	user.vlan = atoi(vlan);
	strncpy(user.user_ip, user_ip, sizeof(user.user_ip) - 1);
	user.auth_type = 1;
	if (os_type == 1)
		con->html_path = "portal/module/pc/mobileAuth.html";
	else
		con->html_path = "portal/module/mobile/mobileAuth.html";
	
	user_query_info_t *query_back = NULL;
	int rcv_len = 0;
	ret = cgi_snd_msg(MSG_CMD_MANAGE_USER_QUERY, &user, sizeof(user), &query_back, &rcv_len);
	if (ret == 0 && query_back->if_exist == 0) {   //0为存在该用户
  		
		html_tag_add(&con->tag_list, "jfwx:tel", query_back->username);
		html_tag_add(&con->tag_list, "jfwx:pwd", query_back->password);
		html_tag_add(&con->tag_list, "jfwx:mac", query_back->mac);
		char tem_vlan[6] = {0};
		snprintf(tem_vlan, 5, "%d", query_back->vlan);
		html_tag_add(&con->tag_list, "jfwx:vlan", tem_vlan);
		html_tag_add(&con->tag_list, "jfwx:user_ip", query_back->user_ip);
		html_tag_add(&con->tag_list, "jfwx:isOld", "1");
	
	} else {
		html_tag_add(&con->tag_list, "jfwx:mac", mac);
		char tem_vlan[6] = {0};
		snprintf(tem_vlan, 5, "%d", user.vlan);
		html_tag_add(&con->tag_list, "jfwx:vlan", tem_vlan);
		html_tag_add(&con->tag_list, "jfwx:user_ip", user.user_ip);
		html_tag_add(&con->tag_list, "jfwx:isOld", "0");
	} 
	if (ret == 0)
		cgi_free_rcvbuf(query_back);
	
out:
	
	return 0;
}

int cgi_sys_user_register_handler(connection_t *con)
{	
	int ret = 1;
	char *name = con_value_get(con,"name");
	char *pwd = con_value_get(con, "pwd");
	char *mac = con_value_get(con,"mac");
	char *vlan = con_value_get(con,"vlan");
	char *user_ip = con_value_get(con, "user_ip");
	CGI_LOG(LOG_DEBUG, "new user register come name: %s, pwd: %s, msc: %s\n", name, pwd, mac);
	user_query_info_t user;
	memset(&user, 0, sizeof(user));
	user.auth_type = 1;
	
	if (!name || !pwd || !mac || !vlan) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		goto out;
	}
		
	strncpy(user.username, name, sizeof(user.username) - 1);
	strncpy(user.password, pwd, sizeof(user.password) - 1);
	strncpy(user.mac, mac, sizeof(user.mac) - 1);
	strncpy(user.user_ip, user_ip, sizeof(user.user_ip) - 1);
	user.vlan = atoi(vlan);
	ret = cgi_snd_msg(MSG_CMD_MANAGE_USER_REGISTER, &user, sizeof(user), NULL, NULL);

	if ( ret== 0) {
		cJSON_AddNumberToObject(con->response, "code", 0);
	} else if (ret == 3){
		cJSON_AddNumberToObject(con->response, "code", 3);
	} else {
		cJSON_AddNumberToObject(con->response, "code", 1);
	}
out:
	return 1;
}

int cgi_sys_text_code_handler(connection_t *con)
{	
	char *name = con_value_get(con,"name");
//	char *mac = con_value_get(con,"mac");

	user_query_info_t user;
	memset(&user, 0, sizeof(user));
	user.auth_type = 1;
	
	if (!name) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		goto out;
	}
		
	strncpy(user.username, name, sizeof(user.username) - 1);
	int code = -1;
	code = cgi_snd_msg(MSG_CMD_MANAGE_TEXT_SEND, &user, sizeof(user), NULL, NULL);
	cJSON_AddNumberToObject(con->response, "code", code);
	
out:
	return 1;
}

int cgi_sys_start_app_handler(connection_t *con)
{	
	char *app = con_value_get(con, "app");
	if (!app) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		goto out;
	}

	
	if (cgi_snd_msg(MSG_CMD_MANAGE_START_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
		cJSON_AddNumberToObject(con->response, "code", 0);
	} else {
		cJSON_AddNumberToObject(con->response, "code", 1);
	}
out:
	return 1;
}

int cgi_sys_stop_app_handler(connection_t *con)
{	
	char *app = con_value_get(con, "app");
	if (!app) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		goto out;
	}

	
	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
		cJSON_AddNumberToObject(con->response, "code", 0);
	} else {
		cJSON_AddNumberToObject(con->response, "code", 1);
	}
out:
	return 1;
}
#endif
int cgi_sys_update_air_pressure_handler(connection_t *con)
{	
	char *pressure = con_value_get(con, "pressure");
	char *client_mac = con_value_get(con, "client_mac");
	char *client_pressure_index = con_value_get(con, "client_pressure_index");
	if (!pressure || !client_mac || !client_pressure_index) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "param not right");
		goto out;
	}
	char sql[256] = {0};
	char *errmsg = NULL;
	snprintf(sql, sizeof(sql) - 1, "INSERT INTO `air_pressure` (client_mac, client_pressure_index, pressure) "
		"VALUES(\"%s\",%s,\"%s\");", client_mac, client_pressure_index, pressure);
	if(SQLITE_OK != sqlite3_exec(pdb,sql,NULL,NULL,&errmsg))
	{
		CGI_LOG(LOG_ERR, "insert record fail!%s\n",errmsg);
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", errmsg);
		goto out;
	}
	cJSON_AddNumberToObject(con->response, "code", 0);

out:
	return 1;
}

int cgi_sys_query_air_pressure_handler(connection_t *con)
{	
	char *period = con_value_get(con, "period");
	char *client_mac = con_value_get(con, "client_mac");
	char *sensor_pin = con_value_get(con, "sensor_pin");
	if (!period || !client_mac || !sensor_pin) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "param not right");
		goto out;
	}
	char sql[256] = {0};
	char *errmsg = NULL;
	char condition[128] = {0};
	if (strcmp(client_mac, "all") != 0)
	{
		snprintf(condition, sizeof(condition), " and client_mac='%s' ", client_mac);
	}
	if (strcmp(sensor_pin, "all") != 0)
	{
		snprintf(condition, sizeof(condition), " and client_pressure_index=%s ", sensor_pin);
	}
	if (strcmp(period, "recent") == 0)
	{
		
		//snprintf(sql, sizeof(sql) - 1, "select * from `temper` where capture_time between datetime('now','start of day','+1 seconds') "
		//	"and  datetime('now','start of day','+1 days','-1 seconds') %s", condition);
		snprintf(sql, sizeof(sql) - 1, "select * from `air_pressure` where capture_time between datetime('now','-1 days', '+1 seconds') "
			"and  datetime('now','-1 seconds') %s", condition);
		
	}
	cJSON *array = cJSON_CreateArray();
	//CGI_LOG(LOG_ERR, "sql:%s\n",sql);
	if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
	{
			CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
			cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
	cJSON_AddItemToObject(con->response, "data", array);

out:
	return 1;
}



int cgi_sys_update_temper_handler(connection_t *con)
{	
	char *temper = con_value_get(con, "temper");
	char *client_mac = con_value_get(con, "client_mac");
	char *sensor_pin = con_value_get(con, "sensor_pin");
	if (!temper || !client_mac || !sensor_pin) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "param not right");
		goto out;
	}
	char sql[256] = {0};
	char *errmsg = NULL;
	/*snprintf(sql, sizeof(sql) - 1, "INSERT INTO `temper` (client_mac, client_temper_index, temper) "
		"VALUES(\"%s\",%s,\"%s\");", client_mac, client_temper_index, temper);*/

	snprintf(sql, sizeof(sql) - 1, "INSERT INTO `temper` (client_mac, sensor_pin, temper, pool_id) "
		"select \"%s\",%s,\"%s\", pool_id from sensor_info where client_mac=%s and sensor_pin=%s;", 
		client_mac, sensor_pin, temper, client_mac, sensor_pin);
	if(SQLITE_OK != sqlite3_exec(pdb,sql,NULL,NULL,&errmsg))
	{
			CGI_LOG(LOG_ERR, "insert record fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
//	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
	cJSON_AddNumberToObject(con->response, "code", 0);
//	} else {
//		cJSON_AddNumberToObject(con->response, "code", 1);
//	}
out:
	return 1;
}

int cgi_sys_update_elec_handler(connection_t *con)
{	
	char *elec = con_value_get(con, "elec");
	if (!elec) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no elec");
		goto out;
	}

	
//	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
	cJSON_AddNumberToObject(con->response, "code", 0);
//	} else {
//		cJSON_AddNumberToObject(con->response, "code", 1);
//	}
out:
	return 1;
}

int cgi_sys_query_elec_handler(connection_t *con)
{	
	char *period = con_value_get(con, "period");
	char *client_mac = con_value_get(con, "client_mac");
	char *client_temper_index = con_value_get(con, "client_temper_index");
	/*if (!elec) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no elec");
		goto out;
	}*/

	
//	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
	cJSON_AddNumberToObject(con->response, "code", 0);
//	} else {
//		cJSON_AddNumberToObject(con->response, "code", 1);
//	}
out:
	return 1;
}

int cgi_sys_update_water_level_handler(connection_t *con)
{	
	char *elec = con_value_get(con, "elec");
	if (!elec) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no elec");
		goto out;
	}

	
//	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
	cJSON_AddNumberToObject(con->response, "code", 0);
//	} else {
//		cJSON_AddNumberToObject(con->response, "code", 1);
//	}
out:
	return 1;
}

int cgi_sys_query_water_level_handler(connection_t *con)
{	
	char *period = con_value_get(con, "period");
	char *client_mac = con_value_get(con, "client_mac");
	char *client_temper_index = con_value_get(con, "client_temper_index");
	/*if (!elec) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no elec");
		goto out;
	}*/

	
//	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
	cJSON_AddNumberToObject(con->response, "code", 0);
//	} else {
//		cJSON_AddNumberToObject(con->response, "code", 1);
//	}
out:
	return 1;
}


int cgi_sys_update_feed_handler(connection_t *con)
{	
	char *feed = con_value_get(con, "feed");
	if (!feed) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no feed");
		goto out;
	}

	cJSON_AddNumberToObject(con->response, "code", 0);

out:
	return 1;
}

int cgi_sys_query_temper_handler(connection_t *con)
{	
	char *period = con_value_get(con, "period");
	char *client_mac = con_value_get(con, "client_mac");
	char *client_temper_index = con_value_get(con, "client_temper_index");
	if (!period || !client_mac || !client_temper_index) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "param not right");
		goto out;
	}
	char sql[256] = {0};
	char *errmsg = NULL;
	char condition[128] = {0};
	if (strcmp(client_mac, "all") != 0)
	{
		snprintf(condition, sizeof(condition), " and client_mac='%s' ", client_mac);
	}
	if (strcmp(client_temper_index, "all") != 0)
	{
		snprintf(condition, sizeof(condition), " and client_temper_index=%s ", client_temper_index);
	}
	if (strcmp(period, "recent") == 0)
	{
		
		//snprintf(sql, sizeof(sql) - 1, "select * from `temper` where capture_time between datetime('now','start of day','+1 seconds') "
		//	"and  datetime('now','start of day','+1 days','-1 seconds') %s", condition);
		snprintf(sql, sizeof(sql) - 1, "select * from `temper` where capture_time between datetime('now','-1 days', '+1 seconds') "
			"and  datetime('now','-1 seconds') %s", condition);
		
	}
	cJSON *array = cJSON_CreateArray();
	//CGI_LOG(LOG_ERR, "sql:%s\n",sql);
	if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
	{
			CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
			cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
//	if (cgi_snd_msg(MSG_CMD_MANAGE_STOP_APP, app, strlen(app) + 1, NULL, NULL) == 0) {
	cJSON_AddNumberToObject(con->response, "code", 0);
	cJSON_AddItemToObject(con->response, "data", array);
//	} else {
//		cJSON_AddNumberToObject(con->response, "code", 1);
//	}
out:
	return 1;
}

int cgi_sys_heart_beat_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");
	if (!client_mac) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac");
		goto out;
	}
	board_info_t *p = NULL;
	task_info_t *task = NULL;
	cJSON *array = NULL, *item = NULL;
	list_for_each_entry(p, &board_list, board_list) {
		if (strcmp(p->mac, client_mac) == 0) {
			p->last_heart_beat_time = uptime();
			if (!list_empty(&p->task_list)) {
				array = cJSON_CreateArray();
				list_for_each_entry(task, &p->task_list, task_list) {
					if (!task->has_been_sent){
						item = cJSON_CreateObject();
						cJSON_AddStringToObject(item, "task_name", task->task_name);
						cJSON_AddNumberToObject(item, "task_id", task->task_id);
						cJSON_AddNumberToObject(item, "report_interval", task->task_report_interval);
						cJSON_AddStringToObject(item, "task_param", task->other_param);
						cJSON_AddItemToArray(array, item);
						task->has_been_sent = 1;
					}
				}
			}
			break;
		}
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
	if (array != NULL) {
		cJSON_AddItemToObject(con->response, "task", array);
	}

out:
	return 1;
}

int cgi_sys_add_task_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");
	char *sensor_pin = con_value_get(con, "sensor_pin");
	char *sensor_type = con_value_get(con, "sensor_type");
	char *report_interval = con_value_get(con, "report_interval");
	char *other_param = con_value_get(con, "other_param");
	if (!client_mac) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac");
		goto out;
	}
	board_info_t *p = NULL;
	task_info_t *task = NULL;
	char sql[256] = {0};
	cJSON *array = NULL, *item = NULL;
	list_for_each_entry(p, &board_list, board_list) {
		if (strcmp(p->mac, client_mac) == 0) {
			snprintf(sql, sizeof(sql) - 1, "update  `sensor_info` set report_interval=%s, other_param='%s' "
				"where client_mac=%s and sensor_pin=%s;",
					report_interval,other_param, client_mac, sensor_pin);
			if(SQLITE_OK != sqlite3_exec(pdb,sql,NULL,NULL,&errmsg))
			{
				CGI_LOG(LOG_ERR, "update sensor_info fail!%s\n",errmsg);
				cJSON_AddNumberToObject(con->response, "code", 1);
				cJSON_AddStringToObject(con->response, "msg", errmsg);
				goto out;
			}
			task_info_t *task_info = malloc(sizeof(task_info_t));
			task_info->task_id = p->task_index;
			p->task_index++;
			task_info->task_report_interval = atoi(report_interval);
			task_info->sensor_type = atoi(sensor_type);
			task_info->sensor_pin = atoi(sensor_pin);
			strncpy(task_info->other_param, other_param, sizeof(task_info->other_param));
			list_add(&task_info->task_list, &p->task_list);
			
			break;
		}
	}
	cJSON_AddNumberToObject(con->response, "code", 0);

out:
	return 1;
}


int cgi_sys_task_result_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");
	char *task_name = con_value_get(con, "task_name");
	char *task_id = con_value_get(con, "task_id");
	char *task_result = con_value_get(con, "task_result");
	if (!client_mac || !task_name || !task_id || !task_result) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "param error");
		goto out;
	}
	board_info_t *p = NULL;
	task_info_t *task = NULL, *task2 = NULL;
	list_for_each_entry(p, &board_list, board_list) {
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
	
	cJSON_AddNumberToObject(con->response, "code", 0);

out:
	return 1;
}


int cgi_sys_add_sensor_info_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");
	//char *board_name = con_value_get(con, "board_name");
	char *sensor_type = con_value_get(con, "sensor_type");
	char *sensor_pin = con_value_get(con, "sensor_pin");
	char *pool_id = con_value_get(con, "pool_id");
	char *report_interval = con_value_get(con, "report_interval");
	char *other_param = con_value_get(con, "other_param");
	char sql[256] = {0};
	char *errmsg = NULL;
	if (!client_mac || !sensor_pin || !sensor_type) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac or no board_name");
		goto out;
	}

	snprintf(sql, sizeof(sql), "select count(*) from sensor_info where client_mac='%s' and sensor_pin=%s;",
	client_mac, sensor_pin);
	if (query_table(sql, 0,0) > 0)
	{
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "sensor pin exist");
		goto out;
	}else
	{
		
		snprintf(sql, sizeof(sql) - 1, "INSERT INTO `sensor_info` (client_mac, sensor_pin, type, pool_id, report_interval, other_param) "
			"VALUES('%s',%d, %d, %d,%d, '%s');", client_mac, atoi(sensor_pin), atoi(sensor_type), atoi(pool_id), atoi(report_interval), other_param);
		if(SQLITE_OK != sqlite3_exec(pdb,sql,NULL,NULL,&errmsg))
		{
				CGI_LOG(LOG_ERR, "insert record fail!%s\n",errmsg);
				cJSON_AddNumberToObject(con->response, "code", 1);
				cJSON_AddStringToObject(con->response, "msg", errmsg);
				goto out;
		}
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
//	}

out:
	return 1;
}

int cgi_sys_update_sensor_info_handler(connection_t *con)
{	
	char *sensor_id = con_value_get(con, "id");
	//char *client_mac = con_value_get(con, "client_mac");
	//char *board_name = con_value_get(con, "board_name");
	char *sensor_type = con_value_get(con, "sensor_type");
	//char *sensor_pin = con_value_get(con, "sensor_pin");
	char *pool_id = con_value_get(con, "pool_id");
	char *report_interval = con_value_get(con, "report_interval");
	char *other_param = con_value_get(con, "other_param");
	char sql[256] = {0};
	char *errmsg = NULL;
	if (!sensor_id) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no id");
		goto out;
	}
//	char *board = v_list_get(&head, client_mac);
//	if (board != NULL)
//	{
//		cJSON_AddNumberToObject(con->response, "code", 1);
//		cJSON_AddStringToObject(con->response, "msg", "board_info exist");
//	}else
//	{
	/*

		esp32_board_t *board = malloc(sizeof(esp32_board_t));
		strncpy(board->mac, client_mac, sizeof(board->mac));
		strncpy(board->name, board_name, sizeof(board->name));
		v_list_add(&head, board->mac, board);
		cJSON_AddNumberToObject(con->response, "code", 0);
		*/
	
	snprintf(sql, sizeof(sql) - 1, "update `sensor_info` set type=%d, pool_id=%d, report_interval=%d, other_param=%s where "
		"id=%s", atoi(sensor_type),  atoi(pool_id),  atoi(report_interval), 
		other_param, sensor_id);
	if(SQLITE_OK != sqlite3_exec(pdb,sql,NULL,NULL,&errmsg))
	{
			CGI_LOG(LOG_ERR, "update record fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
			cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
//	}

out:
	return 1;
}


int cgi_sys_query_sensor_info_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");

	char sql[256] = {0};
	char *errmsg = NULL;
	if (!client_mac) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac or client_mac=all");
		goto out;
	}
	cJSON *array = cJSON_CreateArray();
	snprintf(sql, sizeof(sql), "select * from sensor_info;");
	if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
	{
			CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
			cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
	cJSON_AddItemToObject(con->response, "data", array);
out:
	return 1;
}

int cgi_sys_query_sensor_info_real_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");

	char sql[256] = {0};
	char *errmsg = NULL;
	if (!client_mac) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac or client_mac=all");
		goto out;
	}
	cJSON *array = cJSON_CreateArray();
	snprintf(sql, sizeof(sql), "select * from sensor_info_real;");
	if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
	{
			CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
			cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
	cJSON_AddItemToObject(con->response, "data", array);
out:
	return 1;
}


int cgi_sys_board_start_handler(connection_t *con)
{	
	char *client_mac = con_value_get(con, "client_mac");

	char sql[256] = {0};
	char *errmsg = NULL;
	if (!client_mac) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac or client_mac=all");
		goto out;
	}
	cJSON *array = cJSON_CreateArray();
	snprintf(sql, sizeof(sql), "select * from sensor_info where client_mac='%s';", client_mac);
	if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
	{
			CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
			cJSON_AddNumberToObject(con->response, "code", 1);
			cJSON_AddStringToObject(con->response, "msg", errmsg);
			goto out;
	}
	cJSON_AddNumberToObject(con->response, "code", 0);
	cJSON_AddItemToObject(con->response, "data", array);
out:
	return 1;
}

int cgi_board_report_board_sensor_info(connection_t *con)
{
	char *client_mac = con_value_get(con, "client_mac");
	char *sensor_json = con_value_get(con, "sensor_json");

	char sql[256] = {0};
	int type, sensor_pin, pool_id, report_interval;
	
	char *errmsg = NULL;
	if (!client_mac || !sensor_json) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no client_mac or no sensor_json");
		goto out;
	}
	cJSON *root = cJSON_Parse(sensor_json);
	if (!root)
	{
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "json parse error");
		goto out;
	}
	cJSON *sensor_array = cJSON_GetObjectItem(root, "sensor_array");
	cJSON *array_item = NULL, *item = NULL;
	int sensor_num = cJSON_GetArraySize(sensor_array);
	for (int i=0; i < sensor_num; i++)
	{
		array_item = cJSON_GetArrayItem(sensor_array, i);
		item = cJSON_GetObjectItem(array_item, "sensor_pin");
		sensor_pin = item->valueint;
		item = cJSON_GetObjectItem(array_item, "type");
		type = item->valueint;
		item = cJSON_GetObjectItem(array_item, "pool_id");
		pool_id = item->valueint;
		item = cJSON_GetObjectItem(array_item, "report_interval");
		report_interval = item->valueint;
		snprintf(sql, sizeof(sql) - 1, "replace into `sensor_info_real` (client_mac, sensor_pin, type, pool_id, report_interval) "
					"values('%s', %d, %d, %d, %d)", client_mac, sensor_pin, type, pool_id, report_interval);
		if(SQLITE_OK != sqlite3_exec(pdb,sql,NULL,NULL,&errmsg))
		{
				CGI_LOG(LOG_ERR, "insert record fail!%s\n",errmsg);
				cJSON_AddNumberToObject(con->response, "code", 1);
				cJSON_AddStringToObject(con->response, "msg", errmsg);
				goto out;
		}

	}
	
	cJSON_AddNumberToObject(con->response, "code", 0);
	
out:
	return 1;
}

int cgi_sys_get_boards_status_handler(connection_t *con)
{	

	v_list_t *p;
	v_list_t *q;
	cJSON *board_array, *item;

	board_array = cJSON_CreateArray();
	
	cJSON_AddNumberToObject(con->response, "code", 0);
	cJSON_AddItemToObject(con->response, "data", board_array);

out:
	return 1;
}

int cgi_sys_get_pool_status_handler(connection_t *con)
{	

	char *pool_id = con_value_get(con, "pool_id");
	char *period = con_value_get(con, "period");

	char sql[256] = {0};
	char *errmsg = NULL;
	if (!pool_id || !period) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no pool_id");
		goto out;
	}
	cJSON *array = cJSON_CreateArray();
	if (strcmp(period, "recent") == 0)
	{
		snprintf(sql, sizeof(sql) - 1, "select * from `temper` where capture_time between datetime('now','-1 days', '+1 seconds') "
			"and  datetime('now','-1 seconds') and pool_id=%s", pool_id);
		if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
		{
				CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
				goto error_out;
		}
		cJSON_AddItemToObject(con->response, "temper", array);
		array = cJSON_CreateArray();
		snprintf(sql, sizeof(sql) - 1, "select * from `water_level` where capture_time between datetime('now','-1 days', '+1 seconds') "
			"and  datetime('now','-1 seconds') and pool_id=%s", pool_id);
		if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
		{
				CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
				goto error_out;
		}
		cJSON_AddItemToObject(con->response, "water_level", array);
		array = cJSON_CreateArray();
		snprintf(sql, sizeof(sql) - 1, "select * from `temper` where capture_time between datetime('now','-1 days', '+1 seconds') "
			"and  datetime('now','-1 seconds') and pool_id=%s", pool_id);
		if(SQLITE_OK != sqlite3_exec(pdb, sql, query_data_to_json,(void *)array, &errmsg))
		{
				CGI_LOG(LOG_ERR, "queray fail!%s\n",errmsg);
				goto error_out;
		}
		cJSON_AddItemToObject(con->response, "water_level", array);
		
	}

	cJSON_AddNumberToObject(con->response, "code", 0);
//	cJSON_AddItemToObject(con->response, "data", board_array);

out:
	return 1;
error_out:
	cJSON_AddNumberToObject(con->response, "code", 1);
	cJSON_AddStringToObject(con->response, "msg", errmsg);
	return 1;
	
}

int cgi_sys_img_save_handler(connection_t *con)
{	

	char *camera_id = con_value_get(con, "camera_id");

	char sql[256] = {0};
	char *errmsg = NULL;
	if (!camera_id) {
		cJSON_AddNumberToObject(con->response, "code", 1);
		cJSON_AddStringToObject(con->response, "msg", "no pool_id");
		goto out;
	}
	int ImageMagick; 

	cJSON_AddNumberToObject(con->response, "code", 0);
//	cJSON_AddItemToObject(con->response, "data", board_array);

out:
	return 1;
}










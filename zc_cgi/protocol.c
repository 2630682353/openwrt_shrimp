#include "connection.h"
#include "protocol.h"

#define PRO_UPDATE_ELEC  "update_elec"
#define PRO_QUERY_ELEC  "query_elec"

#define PRO_UPDATE_TEMPER  "update_temper"
#define PRO_QUERY_TEMPER  "query_temper"

#define PRO_UPDATE_AIR_PRESSURE "update_air_pressure"
#define PRO_QUERY_AIR_PRESSURE "query_air_pressure"

#define PRO_UPDATE_PH "update_ph"
#define PRO_QUERY_PH "query_ph"

#define PRO_UPDATE_WATER_LEVEL "update_water_level"
#define PRO_QUERY_WATER_LEVEL "query_water_level"

#define PRO_UPDATE_LIGHT_LUX "update_light_lux"
#define PRO_QUERY_LIGHT_LUX "query_light_lux"

#define PRO_UPDATE_FEED_WEIGHT "update_feed_weight"
#define PRO_QUERY_FEED_WEIGHT "query_feed_weight"

#define PRO_UPDATE_TRANSPARENT "update_transparent"
#define PRO_QUERY_TRANSPARENT "query_transparent"

#define PRO_QUERY_SENSOR  "query_sensor"
#define PRO_DELETE_SENSOR  "delete_sensor"
#define PRO_UPDATE_SENSOR  "update_sensor"
#define PRO_QUERY_SENSOR_REAL  "query_sensor_real"

#define PRO_HEART_BEAT "heart_beat"
#define PRO_ADD_SENSOR "add_sensor"
#define PRO_GET_BOARDS_STATUS "get_boards_status"
#define PRO_POOL_STATUS "pool_status"
#define PRO_IMG_SAVE "img_save"
#define PRO_QUERY_SENSOR_BY_MAC "query_sensor_by_mac"
#define PRO_REPORT_BOARD_SENSOR "report_board_sensor"
#define PRO_ADD_TASK "add_task"
#define PRO_TASK_RESULT "task_result"

extern int cgi_sys_update_elec_handler(connection_t *con);
extern int cgi_sys_query_elec_handler(connection_t *con);

extern int cgi_sys_update_ph_handler(connection_t *con);
extern int cgi_sys_query_ph_handler(connection_t *con);

extern int cgi_sys_update_air_pressure_handler(connection_t *con);
extern int cgi_sys_query_air_pressure_handler(connection_t *con);

extern int cgi_sys_update_temper_handler(connection_t *con);  
extern int cgi_sys_query_temper_handler(connection_t *con);

extern int cgi_sys_update_water_level_handler(connection_t *con);  
extern int cgi_sys_query_water_level_handler(connection_t *con);

extern int cgi_sys_update_transparent_handler(connection_t *con);  
extern int cgi_sys_query_transparent_handler(connection_t *con);

extern int cgi_sys_update_light_lux_handler(connection_t *con);  
extern int cgi_sys_query_light_lux_handler(connection_t *con);

extern int cgi_sys_update_feed_weight_handler(connection_t *con);
extern int cgi_sys_query_feed_weight_handler(connection_t *con);

extern int cgi_sys_heart_beat_handler(connection_t *con);
extern int cgi_sys_add_sensor_info_handler(connection_t *con);
extern int cgi_sys_delete_sensor_handler(connection_t *con);
extern int cgi_sys_query_sensor_info_handler(connection_t *con);
extern int cgi_sys_query_sensor_info_real_handler(connection_t *con);


extern int cgi_sys_get_boards_status_handler(connection_t *con);
extern int cgi_sys_get_pool_status_handler(connection_t *con);

extern int cgi_sys_img_save_handler(connection_t *con);
extern int cgi_sys_query_sensor_info_by_mac_handler(connection_t *con);
extern int cgi_board_report_board_sensor_info(connection_t *con);
extern int cgi_sys_add_task_handler(connection_t *con);
extern int cgi_sys_task_result_handler(connection_t *con);
extern int cgi_sys_update_sensor_info_handler(connection_t *con);


static cgi_protocol_t pro_list[] ={
	{PRO_UPDATE_ELEC, cgi_sys_update_elec_handler},
	{PRO_QUERY_ELEC, cgi_sys_query_elec_handler},
	{PRO_UPDATE_TEMPER, cgi_sys_update_temper_handler},
	{PRO_QUERY_TEMPER, cgi_sys_query_temper_handler},
	{PRO_UPDATE_WATER_LEVEL, cgi_sys_update_water_level_handler},
	{PRO_QUERY_WATER_LEVEL, cgi_sys_query_water_level_handler},
	{PRO_UPDATE_FEED_WEIGHT, cgi_sys_update_feed_weight_handler},
	{PRO_QUERY_FEED_WEIGHT, cgi_sys_query_feed_weight_handler},
	{PRO_HEART_BEAT, cgi_sys_heart_beat_handler},
	{PRO_ADD_SENSOR, cgi_sys_add_sensor_info_handler},
	{PRO_DELETE_SENSOR, cgi_sys_delete_sensor_handler},
	{PRO_QUERY_SENSOR, cgi_sys_query_sensor_info_handler},
	{PRO_UPDATE_SENSOR, cgi_sys_update_sensor_info_handler},
	{PRO_QUERY_SENSOR_REAL, cgi_sys_query_sensor_info_real_handler},
	{PRO_GET_BOARDS_STATUS, cgi_sys_get_boards_status_handler},
	{PRO_QUERY_AIR_PRESSURE, cgi_sys_query_air_pressure_handler},
	{PRO_UPDATE_AIR_PRESSURE, cgi_sys_update_air_pressure_handler},
	{PRO_POOL_STATUS, cgi_sys_get_pool_status_handler},
	{PRO_IMG_SAVE, cgi_sys_img_save_handler},
	{PRO_QUERY_SENSOR_BY_MAC, cgi_sys_query_sensor_info_by_mac_handler},
	{PRO_REPORT_BOARD_SENSOR, cgi_board_report_board_sensor_info},
	{PRO_ADD_TASK, cgi_sys_add_task_handler},
	{PRO_TASK_RESULT, cgi_sys_task_result_handler},
	{NULL,NULL},
};

int cgi_protocol_handler(connection_t *con)
{
	cJSON *obj;
	cgi_protocol_t *cur_protocol;
	char* opt = con_value_get(con,"opt");
	//char* fname = con_value_get(con,"fname");

	if(!opt)
		return PRO_BASE_ARG_ERR; 

	cur_protocol = find_pro_handler(opt);
	if(!cur_protocol)
		return PRO_BASE_ARG_ERR;

/*

	cJSON_AddStringToObject(response,"opt", opt);

	if (connection_is_set(con) == -1)
		return PRO_BASE_ARG_ERR; 
	if (connection_is_set(con)) {
		cJSON_AddStringToObject(response,"function", "set");
	} else {
		cJSON_AddStringToObject(response,"function", "get");
	}
*/
	return cur_protocol->handler(con);
}

cgi_protocol_t *find_pro_handler(const char *pro_opt)
{
    int i;
    if(pro_opt == NULL)
        return NULL;
    i = 0;
    while(1){
        if(pro_list[i].name == NULL)
            return NULL;
        if(strcmp(pro_list[i].name, pro_opt) == 0){
            return &pro_list[i];
        }
        i++;
    }
        return NULL;
}

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

#define PRO_UPDATE_TRANSPARENT "update_transparent"
#define PRO_QUERY_TRANSPARENT "query_transparent"

#define PRO_QUERY_SENSOR  "query_sensor"
#define PRO_HEART_BEAT "heart_beat"
#define PRO_ADD_SENSOR "add_sensor"
#define PRO_GET_BOARDS_STATUS "get_boards_status"


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

extern int cgi_sys_heart_beat_handler(connection_t *con);
extern int cgi_sys_add_sensor_info_handler(connection_t *con);
extern int cgi_sys_query_sensor_info_handler(connection_t *con);
extern int cgi_sys_get_boards_status_handler(connection_t *con);


static cgi_protocol_t pro_list[] ={
	{PRO_UPDATE_ELEC, cgi_sys_update_elec_handler},
	{PRO_QUERY_ELEC, cgi_sys_query_elec_handler},
	{PRO_UPDATE_TEMPER, cgi_sys_update_temper_handler},
	{PRO_QUERY_TEMPER, cgi_sys_query_temper_handler},
	{PRO_UPDATE_WATER_LEVEL, cgi_sys_update_water_level_handler},
	{PRO_QUERY_WATER_LEVEL, cgi_sys_query_water_level_handler},
	{PRO_HEART_BEAT, cgi_sys_heart_beat_handler},
	{PRO_ADD_SENSOR, cgi_sys_add_sensor_info_handler},
	{PRO_QUERY_SENSOR, cgi_sys_query_sensor_info_handler},
	{PRO_GET_BOARDS_STATUS, cgi_sys_get_boards_status_handler},
	{PRO_QUERY_AIR_PRESSURE, cgi_sys_query_air_pressure_handler},
	{PRO_UPDATE_AIR_PRESSURE, cgi_sys_update_air_pressure_handler},
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

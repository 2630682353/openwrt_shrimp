#ifndef __connection_h_
#define __connection_h_
#include <stdlib.h>
#include "cJSON.h"
#include "list.h"
#include <time.h>

typedef struct v_list{
	char *key;
	char *value;
	struct v_list *next;
}v_list_t;

typedef struct html_tag{
	char *key;
	char *value;
	struct list_head list;
}html_tag_t;

typedef struct connection{
	v_list_t head;
	struct list_head tag_list;
	cJSON *response;
	void (*free)(struct connection*);
	int function;
	char *html_path;
	char *out_str;
}connection_t;

typedef struct board_info{
	char mac[32];
	char name[16];
	time_t last_heart_beat_time;
	int task_index;
	struct list_head task_list;
	struct list_head board_list;
}board_info_t;

typedef struct task_info{
	char task_name[16];
	int task_id;
	int has_been_sent;
	int task_report_interval;
	char other_param[32];
	struct list_head task_list;
}task_info_t;

extern void connection_init(connection_t *con);
extern void connection_parse(connection_t *con, char *src);
extern char *con_value_get(connection_t *con, char*key);
extern int html_tag_add(struct list_head *list, char *key, char *value);
#endif
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "message.h"
#include "sock.h"
#include "log.h"
#include "protocol.h"
#include "connection.h"
#include    "uci_fn.h"
#include <fcgi_stdio.h>
#include <sqlite3.h>
sqlite3 *pdb = NULL;
v_list_t head;

int cgi_init()
{
	char **array = NULL;
	int num = 0;
	if (!uuci_get("gateway_config.gateway_base.portal_cgi_loglevel", &array, &num)) {
		log_leveljf = atoi(array[0]);
		uuci_get_free(array, num);
	}
}


struct list_head board_list;
extern int query_multi_result(char *sql, char out[][]);

char board_mac[30][32];


int main()
{

/*
	char *str_len = NULL;
	int len = 0;
	char buf[100] = {0};
	user_info_t user;
	cJSON *root;
	char *out = NULL;	
	int ret = -1;
	
	str_len = getenv("CONTENT_LENGTH");
	if ((str_len == NULL) || (sscanf(str_len, "%d", &len)!=1) || (len>80)) {
	
		root = cJSON_CreateObject();
		cJSON_AddNumberToObject(root,"login",0);
		cJSON_AddNumberToObject(root,"error",CGI_ERR_OTHER);
		goto reply_print;
	}
	fgets(buf, len+1, stdin);
	memset(&user, 0, sizeof(user_info_t));
	sscanf(buf, "name=%[^&]&password=%s",user.name,user.pwd);
	memset(user.mac, 0xFF, 6);
	root = cJSON_CreateObject();
	ret = auth_handle(&user);
	cJSON_AddNumberToObject(root,"login",0);
	cJSON_AddNumberToObject(root,"error",ret);

reply_print:
	printf("%s\r\n\r\n","Content-Type:application/json;charset=UTF-8"); 

	out=cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n", out);
	if (out)
		free(out);
*/
	if (pdb == NULL) {
		if(SQLITE_OK != sqlite3_open("/home/work/test.db3",&pdb))
		{
				CGI_LOG(LOG_ERR, "open dtabase fail!%s\n",sqlite3_errmsg(pdb));
				exit(EXIT_FAILURE);
		}

	}
	for (int i=0;i < 30;i++)
	{
		memset(board_mac[i], 0, 32);
	}
	int num = query_multi_result("select distinct client_mac from sensor_info", board_mac);
	INIT_LIST_HEAD(&board_list);
	for (int i = 0; i < num;i++)
	{
		board_info_t *board = malloc(sizeof(board_info_t));
		memset(board, 0, sizeof(board));
		strncpy(board->mac, board_mac[i], 32);
		list_add(&board->board_list, &board_list);
		CGI_LOG("board_mac:%s", board->mac);
	}
	while(FCGI_Accept() >= 0)
	{
		//cgi_init();
		if (pdb == NULL) {
			if(SQLITE_OK != sqlite3_open("/home/work/test.db3",&pdb))
			{
					CGI_LOG(LOG_ERR, "open dtabase fail!%s\n",sqlite3_errmsg(pdb));
					exit(EXIT_FAILURE);
			}

		}	
		connection_t con;
		connection_init(&con);
		connection_handel(&con);
		printf("%s", con.out_str);
		con.free(&con);	
	}
}
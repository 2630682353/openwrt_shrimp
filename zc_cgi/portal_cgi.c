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

typedef struct board_info{
	char *mac;
	const char *name;
}board_info_t;

board_info_t board_mac[] = {
					{"aa:aa:aa:aa:aa:aa", "no.1_pool"},
					{"bb:bb:bb:bb:bb:bb", "no.2_pool"}
					};


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
	v_list_init(&head);

	for (int i = 0; i< sizeof(board_mac)/sizeof(board_mac[0]);i++)
	{
		esp32_board_t *board = malloc(sizeof(esp32_board_t));
		memset(board, 0, sizeof(esp32_board_t));
		strncpy(board->mac, board_mac[i].mac, sizeof(board->mac));
		strncpy(board->name, board_mac[i].name, sizeof(board->name));
		v_list_add(&head, board->mac, board);
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
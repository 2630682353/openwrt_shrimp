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
#include <timer.h>
sqlite3 *pdb = NULL;
v_list_t head;
int pipefd[2];

int cgi_init()
{
	char **array = NULL;
	int num = 0;
	if (!uuci_get("gateway_config.gateway_base.portal_cgi_loglevel", &array, &num)) {
		log_leveljf = atoi(array[0]);
		uuci_get_free(array, num);
	}
}


int check_air_pressure_handle(void *para)
{
	CGI_LOG(LOG_ERR, "timer_handle");
	return 0;
}

struct list_head board_list;
extern int query_multi_result(char *sql, char *out[]);

char board_mac[30][32];
pthread_t my_thread;
void *my_thread_thread_cb(void *arg)
{
	socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
	int ret = 0;
	struct  timeval  timeout = {10,0};
	int cmds[100];
	setsockopt(pipefd[0], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));
	timer_list_init();
	add_timer(check_air_pressure_handle, 2, 1, 60, NULL, 0);
	while(1)
	{
		ret = recv(pipefd[0], cmds, 100, 0);
		if (ret > 0) {
	      for(int i = 0; i < ret; i++) {
	        switch(cmds[i]) {
	        case 1:
	          break;
	        }
	      }
	    }
		
	}

}

int main()
{

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
		strncpy(board->mac, &board_mac[i][0], 32);
		list_add(&board->board_list, &board_list);
		CGI_LOG(LOG_ERR,"board_mac:%s", board->mac[i]);
	}
	pthread_create(&my_thread), NULL, my_thread_thread_cb, NULL);
	while(FCGI_Accept() >= 0)
	{
		connection_t con;
		connection_init(&con);
		connection_handel(&con);
		printf("%s", con.out_str);
		con.free(&con);	
	}
}
#include "ZT_Inc.h"
#include "ZT_ctrl.h"
#include "ZT_sock.h"
#include <stdio.h>
#include <strings.h>
#include <pthread.h>

/*----------------------------------------------------------------------
 *    Global Variable
 ----------------------------------------------------------------------*/

char                gstr_read_line [128] = {'\0',};
extern int          gi_pcap_open;
static int          g_socket_fd = -1;

st_commands_t       gp_commands [] = {

    {"[common]",0,""},
    {"help",0,"This Screen"},
    {"signup",3,"User signup"},
    {"login",2,"User login"},
    {"history",0,"History"},
    {"log",1,"on/off : Log (default off)"},
    {"quit",1,"Program quit"},
    {NULL, 0, NULL}
};



void CTRL_proc(int argc, char **argv)
{
	if (argc < 1)
		return;

	if (!strcasecmp(argv[0], "signup")) 
	{
		user_t t_user = {0};
		int rc;

		if (g_socket_fd < 0) {
			printf("[signup] Socket not connected\n");
			return;
		}

		if (argc < 4) {
			printf("[signup] Usage: signup <user_id> <name> <password>\n");
			return;
		}
		snprintf(t_user.str_user_id, sizeof(t_user.str_user_id), "%.15s", argv[1]);
		snprintf(t_user.str_name, sizeof(t_user.str_name), "%.15s", argv[2]);
		snprintf(t_user.str_pwd, sizeof(t_user.str_pwd), "%.15s", argv[3]);

		rc = Join(g_socket_fd, &t_user);
		if (rc == 0)
			printf("[signup] Join request sent\n");
		else
			printf("[signup] Join send fail\n");
		return;
	}

	if (!strcasecmp(argv[0], "login"))
	{
		int rc;

		if (g_socket_fd < 0) {
			printf("[login] Socket not connected\n");
			return;
		}

		if (argc < 3) {
			printf("[login] Usage: login <user_id> <password>\n");
			return;
		}

		rc = Login(g_socket_fd, argv[1], argv[2]);
		if (rc == 0)
			printf("[login] Login request sent\n");
		else
			printf("[login] Login send fail\n");
		return;
	}
	
	if (!strcasecmp(argv[0], "help")) 
	{
		int i = 0;
		printf("================================================\n");
		printf("%-12s  %s\n", "Command", "Description");
		printf("================================================\n");

		while(gp_commands[i].name)
		{
			printf("%-12s  %s\n", gp_commands[i].name, gp_commands[i].doc);
			i++;
		}
		return;
	}
	
	if (!strcasecmp(argv[0], "quit") || !strcasecmp(argv[0], "exit")) 
	{
		printf("[CMD_LINE] quit\n");
		exit(0);
	} 
	else {
		printf("[CMD_LINE] unknown command: %s (try 'help')\n", argv[0]);
	}
}

static int parse_cmd_line(char *line, char **argv, int max_argc)
{
    int argc = 0;
    char *p = line;

    while (argc < max_argc)
	{
        while (*p == ' ' || *p == '\t' || *p == '\n') 
			*p++ = '\0';

        if (!*p) 
			break;

		argv[argc++] = p;
        
		while (*p && *p != '\n') 
			p++;
        
		if (*p == '\n') 
			*p++ = '\0';
    }

    return argc;
}

static void *CTRL_handler(void *ctx)
{
	static char line_buf[MAX_CMD_LEN];
	static char signup_buf[3][64];
	static char login_buf[2][64];
	char *argv[MAX_ARGC];
	int nargs, i, need;
	char *p;

	(void)ctx;

	while (1) 
	{
		printf("> ");
		fflush(stdout);

		if (!fgets(line_buf, sizeof(line_buf), stdin))
			break;

		nargs = parse_cmd_line(line_buf, argv, MAX_ARGC);
		if (nargs < 1)
			continue;

		if (!strcasecmp(argv[0], "signup") && nargs < 4)
		{
			const char *prompts[] = {"user_id: ", "name: ", "password: "};
			need = 4 - nargs;

			for (i = 0; i < need && nargs < 4; i++)
			{
				printf("%s", prompts[nargs - 1]);
				fflush(stdout);
				if (!fgets(signup_buf[i], sizeof(signup_buf[i]), stdin))
					break;
				p = strchr(signup_buf[i], '\n');
				if (p) *p = '\0';
				argv[nargs] = signup_buf[i];
				nargs++;
			}
		}

		if (!strcasecmp(argv[0], "login") && nargs < 3)
		{
			const char *prompts[] = {"user_id: ", "password: "};
			need = 3 - nargs;

			for (i = 0; i < need && nargs < 3; i++)
			{
				printf("%s", prompts[nargs - 1]);
				fflush(stdout);
				if (!fgets(login_buf[i], sizeof(login_buf[i]), stdin))
					break;
				p = strchr(login_buf[i], '\n');
				if (p) *p = '\0';
				argv[nargs] = login_buf[i];
				nargs++;
			}
		}

		CTRL_proc(nargs, argv);
	}

	return NULL;
}

int CTRL_start(int socket)
{
	pthread_t t_id;

	g_socket_fd = socket;

	if(pthread_create(&t_id, NULL, CTRL_handler, (void*)NULL) < 0)
	{
		printf("[ERROR] pthread_create Fail\n");
		return -1;
	}

	return 0;
}

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
    {"help",0,"                                         : This Screen"},
    {"signup",4,"  <user_id> <name> <phone> <email>     : User signup"},
    {"history",0,"                                      : History"},
    {"log",1,"     on/off                               : Log (default off)"},
    {"quit",1,"                                         : Program quit"},
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

		snprintf(t_user.str_user_id, sizeof(t_user.str_user_id), "%.15s", argv[1]);
		snprintf(t_user.str_name, sizeof(t_user.str_name), "%.15s", argv[2]);
		snprintf(t_user.str_email, sizeof(t_user.str_email), "%.63s", argv[3]);

		rc = Join(g_socket_fd, &t_user);
		if (rc == 0)
			printf("[signup] Join request sent\n");
		else
			printf("[signup] Join send fail\n");
		return;
	} 
	
	if (!strcasecmp(argv[0], "help")) 
	{
		int i = 0;
		printf("================================================\n");
		printf("Command\t\t Description\n");
		printf("================================================\n");

		while(gp_commands[i].name)
		{
			printf("%s\t\t%s\n", gp_commands[i].name, gp_commands[i].doc);
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
        while (*p == ' ' || *p == '\t') 
			*p++ = '\0';

        if (!*p || *p == '\n') 
			break;

		argv[argc++] = p;
        
		while (*p && *p != ' ' && *p != '\t' && *p != '\n') 
			p++;
        
		if (*p == '\n') 
		{ 
			*p = '\0'; 
			break; 
		}
    }

    return argc;
}

static void *CTRL_handler(void *ctx)
{
	struct epoll_event ev;
	struct epoll_event t_ev[MAX_EVENTS];
	static char line_buf[MAX_CMD_LEN];
	static int line_len = 0;
	
	char *argv[MAX_ARGC];
	int nargs, n;

	int epfd = -1;
	int i, rc = 0;

	epfd = epoll_create1(0);
	if (epfd < 0) {
		printf("[CTRL_handler] epoll_create Fail\n");
		return (void *)(intptr_t)ERR_EPOLL_CREATE;
	}

	rc = SET_NONBLOCKING(STDIN_FILENO);
	if (rc == 0) 
	{
		ev.events = EPOLLIN;
		ev.data.fd = STDIN_FILENO;

		/* stdin registed */
		rc = epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
	}

	while (1) 
	{
		n = epoll_wait(epfd, t_ev, MAX_EVENTS, -1);
		if (n < 0) 
		{
			if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
				continue;

			printf("[EventLoop] epoll_wait Fail\n");
			goto close_event;
		}

		for (i = 0; i < n; i++)
		{
			if(t_ev[i].data.fd == STDIN_FILENO)
			{
				while(1)
				{
					rc = read(STDIN_FILENO, line_buf, sizeof(line_buf) - line_len - 1);
					if(rc <= 0)
					{
						if(errno == EAGAIN)
							break;

						goto close_event;
					}

					line_len += rc;
					line_buf[line_len] = '\0';

					if(strchr(line_buf, '\n'))
					{
						nargs = parse_cmd_line(line_buf, argv, MAX_ARGC);

						if(nargs >= 0)
						{
							/* cmd process */
							CTRL_proc(nargs, argv);
						}

						/* initial len size */
						line_len = 0;
					}

				}
			}

		}
	}

	return NULL;

close_event:

	if (epfd >= 0)
		close(epfd);
	
	return (void *)(intptr_t)-1;
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
